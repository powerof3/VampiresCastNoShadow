static RE::BGSKeyword* vampireKYWD;

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

	struct FinishHeadAttach
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

	static void Patch()
	{
		REL::Relocation<std::uintptr_t> attach_armor{ REL::ID(15678) };
		stl::write_thunk_call<AttachBSFadeNode>(attach_armor.address() + 0xB70);

		//torches/weapons/anything with TESMODEL
		REL::Relocation<std::uintptr_t> attach_weapon{ REL::ID(15746) };
		stl::write_thunk_call<AttachBSFadeNode>(attach_weapon.address() + 0x2EC);

		REL::Relocation<std::uintptr_t> attach_head{ REL::ID(24732) };
		stl::write_thunk_call<FinishHeadAttach>(attach_head.address() + 0x15B);
	}
}

void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
		if (vampireKYWD = RE::TESForm::LookupByID<RE::BGSKeyword>(0x000A82BB); vampireKYWD) {
			SKSE::AllocTrampoline(28);
			VampiresCastNoShadow::Patch();
		}
	}
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName("Vampires Cast No Shadow");
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("loaded plugin");
	
	SKSE::Init(a_skse);

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener("SKSE", OnInit);

	return true;
}
