static RE::BGSKeyword* vampireKYWD{ nullptr };

namespace VampiresCastNoShadow
{
	struct detail
	{
		static void stop_shadow_cast(RE::NiAVObject* a_object)
		{
			using FLAG = RE::BSShaderProperty::EShaderPropertyFlag8;

			RE::BSVisit::TraverseScenegraphGeometries(a_object, [&](RE::BSGeometry* a_geometry) -> RE::BSVisit::BSVisitControl {
				const auto effect = a_geometry->properties[RE::BSGeometry::States::kEffect];
				const auto lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(effect.get());
				if (lightingShader) {
					lightingShader->SetFlags(FLAG::kCastShadows, false);
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
		}
	};

	struct AttachBSFadeNode
	{
		static void thunk(RE::NiAVObject* a_object, RE::BSFadeNode* a_root3D)
		{
			func(a_object, a_root3D);

			const auto user = a_root3D && a_object ? a_root3D->GetUserData() : nullptr;
			const auto actor = user ? user->As<RE::Actor>() : nullptr;

			if (actor && actor->HasKeyword(vampireKYWD)) {
				detail::stop_shadow_cast(a_object);
			}
		}
		static inline REL::Relocation<decltype(&thunk)> func;
	};

	struct StoreHeadNodes
	{
		static void thunk(RE::Actor* a_actor, RE::NiAVObject* a_root, RE::BSFaceGenNiNode* a_faceNode)
		{
			func(a_actor, a_root, a_faceNode);

			if (a_actor && a_faceNode && a_actor->HasKeyword(vampireKYWD)) {
				detail::stop_shadow_cast(a_faceNode);
			}
		}
		static inline REL::Relocation<decltype(&thunk)> func;
	};

	void Patch()
	{
		REL::Relocation<std::uintptr_t> attach_armor{ RELOCATION_ID(15501, 15678), OFFSET(0xA13, 0xB70) };
		stl::write_thunk_call<AttachBSFadeNode>(attach_armor.address());

		//torches/weapons/anything with TESMODEL
		REL::Relocation<std::uintptr_t> attach_weapon{ RELOCATION_ID(15569, 15746), OFFSET(0x2DD, 0x2EC) };
		stl::write_thunk_call<AttachBSFadeNode>(attach_weapon.address());

		REL::Relocation<std::uintptr_t> attach_head{ RELOCATION_ID(24228, 24732), OFFSET(0x1CD, 0x15B) };
		stl::write_thunk_call<StoreHeadNodes>(attach_head.address());
	}
}

void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		VampiresCastNoShadow::Patch();
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		vampireKYWD = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("Vampire");
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Vampires Cast No Shadow");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary();
	v.UsesNoStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = "Vampires Cast No Shadow";
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("loaded");

	SKSE::Init(a_skse);

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", OnInit);

	return true;
}
