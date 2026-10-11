// VanillaDirectionalDodge
// РќР°Р¶Р°С‚РёРµ РєР»Р°РІРёС€Рё -> РїРѕРІРѕСЂРѕС‚ РїРµСЂСЃРѕРЅР°Р¶Р° РІ СЃС‚РѕСЂРѕРЅСѓ РґРІРёР¶РµРЅРёСЏ (WASD РѕС‚РЅРѕСЃРёС‚РµР»СЊРЅРѕ РєР°РјРµСЂС‹)
// -> РІСЂРµРјРµРЅРЅС‹Р№ РІС…РѕРґ РІ СЃРєСЂС‹С‚РЅРѕСЃС‚СЊ -> РІР°РЅРёР»СЊРЅС‹Р№ РїРµСЂРµРєР°С‚ -> РІРѕР·РІСЂР°С‚ СЃРѕСЃС‚РѕСЏРЅРёСЏ Рё РєР°РјРµСЂС‹.

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
	constexpr auto kIniPath = "Data\\SKSE\\Plugins\\VanillaDirectionalDodge.ini";

	// ---------------------------------------------------------------- config
	struct Config
	{
		std::uint32_t key = 47;          // DirectInput scancode, 47 = V
		std::uint32_t cooldownMs = 800;  // РїР°СѓР·Р° РјРµР¶РґСѓ РїРµСЂРµРєР°С‚Р°РјРё
		bool rotateToInput = true;       // СЂР°Р·РІРѕСЂР°С‡РёРІР°С‚СЊ РїРµСЂСЃРѕРЅР°Р¶Р° РїРѕ WASD
		std::vector<std::string> events{ "SneakSprintStartRoll" };

		// РџРµСЂРµРєР°С‚ РїСЂРёРЅРёРјР°РµС‚СЃСЏ РіСЂР°С„РѕРј Р°РЅРёРјР°С†РёР№ С‚РѕР»СЊРєРѕ РІ СЃРѕСЃС‚РѕСЏРЅРёРё СЃРєСЂС‹С‚РЅРѕСЃС‚Рё,
		// РїРѕСЌС‚РѕРјСѓ РІРЅРµ СЃРєСЂС‹С‚РЅРѕСЃС‚Рё РјС‹ РІСЂРµРјРµРЅРЅРѕ РІС…РѕРґРёРј РІ РЅРµС‘ Рё РїРѕС‚РѕРј РІС‹С…РѕРґРёРј.
		bool tempSneak = true;
		std::uint32_t sneakEnterDelayMs = 120;  // РѕР¶РёРґР°РЅРёРµ, РїРѕРєР° РіСЂР°С„ РїРµСЂРµР№РґС‘С‚ РІ sneak-СЃРѕСЃС‚РѕСЏРЅРёРµ
		std::uint32_t rollDurationMs = 1000;    // СЃРєРѕР»СЊРєРѕ Р¶РґР°С‚СЊ РґРѕ РІС‹С…РѕРґР° РёР· СЃРєСЂС‹С‚РЅРѕСЃС‚Рё
		bool restoreCamera = true;              // РІРµСЂРЅСѓС‚СЊ РІРёРґ (1-Рµ/3-Рµ Р»РёС†Рѕ) Рё РїРѕРІРѕСЂРѕС‚ РєР°РјРµСЂС‹
		bool logGraphEvents = false;            // РїРёСЃР°С‚СЊ РІ Р»РѕРі СЃРѕР±С‹С‚РёСЏ РіСЂР°С„Р° (РґР»СЏ РѕС‚Р»Р°РґРєРё)
		std::string sneakStartEvent = "SneakStart";
		std::string sneakStopEvent = "SneakStop";

		void Load()
		{
			key = ::GetPrivateProfileIntA("General", "Key", static_cast<int>(key), kIniPath);
			cooldownMs = ::GetPrivateProfileIntA("General", "CooldownMs", static_cast<int>(cooldownMs), kIniPath);
			rotateToInput = ::GetPrivateProfileIntA("General", "RotateToInput", 1, kIniPath) != 0;

			tempSneak = ::GetPrivateProfileIntA("General", "TemporarySneak", 1, kIniPath) != 0;
			sneakEnterDelayMs = ::GetPrivateProfileIntA("General", "SneakEnterDelayMs", static_cast<int>(sneakEnterDelayMs), kIniPath);
			rollDurationMs = ::GetPrivateProfileIntA("General", "RollDurationMs", static_cast<int>(rollDurationMs), kIniPath);
			restoreCamera = ::GetPrivateProfileIntA("General", "RestoreCamera", 1, kIniPath) != 0;
			logGraphEvents = ::GetPrivateProfileIntA("General", "LogGraphEvents", 0, kIniPath) != 0;

			char evBuf[128]{};
			::GetPrivateProfileStringA("General", "SneakStartEvent", "SneakStart", evBuf, sizeof(evBuf), kIniPath);
			sneakStartEvent = evBuf;
			::GetPrivateProfileStringA("General", "SneakStopEvent", "SneakStop", evBuf, sizeof(evBuf), kIniPath);
			sneakStopEvent = evBuf;

			char buf[512]{};
			::GetPrivateProfileStringA("General", "RollEvents", "SneakSprintStartRoll", buf, sizeof(buf), kIniPath);

			events.clear();
			std::stringstream ss(buf);
			for (std::string item; std::getline(ss, item, ',');) {
				const auto b = item.find_first_not_of(" \t");
				const auto e = item.find_last_not_of(" \t");
				if (b != std::string::npos) {
					events.emplace_back(item.substr(b, e - b + 1));
				}
			}
			if (events.empty()) {
				events.emplace_back("SneakSprintStartRoll");
			}
		}
	};

	Config g_cfg;

	void SetupLog()
	{
		auto path = SKSE::log::log_directory();
		if (!path) {
			return;
		}
		*path /= "VanillaDirectionalDodge.log";

		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global", std::move(sink));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S] [%l] %v");
	}

	// ---------------------------------------------------------------- dodge
	std::optional<float> GetCameraYaw()
	{
		const auto* cam = RE::Main::WorldRootCamera();
		if (!cam) {
			return std::nullopt;
		}
		// РєР°РјРµСЂР° Skyrim СЃРјРѕС‚СЂРёС‚ РІРґРѕР»СЊ Р»РѕРєР°Р»СЊРЅРѕР№ РѕСЃРё Y
		const auto& r = cam->world.rotate;
		return std::atan2(r.entry[0][1], r.entry[1][1]);
	}

	float NormalizeAngle(float a)
	{
		constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
		a = std::fmod(a, twoPi);
		return a < 0.0f ? a + twoPi : a;
	}

	// РІС‹РїРѕР»РЅРёС‚СЊ РґРµР№СЃС‚РІРёРµ РІ РіР»Р°РІРЅРѕРј РїРѕС‚РѕРєРµ РёРіСЂС‹ С‡РµСЂРµР· delayMs
	template <class F>
	void RunLater(std::uint32_t delayMs, F&& fn)
	{
		std::thread([delayMs, fn = std::forward<F>(fn)]() mutable {
			std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
			SKSE::GetTaskInterface()->AddTask(std::move(fn));
		}).detach();
	}

	// ---- РѕС‚Р»Р°РґРѕС‡РЅС‹Р№ РїСЂРёС‘РјРЅРёРє СЃРѕР±С‹С‚РёР№ РіСЂР°С„Р° Р°РЅРёРјР°С†РёР№ РёРіСЂРѕРєР°
	class GraphSink final : public RE::BSTEventSink<RE::BSAnimationGraphEvent>
	{
	public:
		static GraphSink* GetSingleton()
		{
			static GraphSink singleton;
			return std::addressof(singleton);
		}

		RE::BSEventNotifyControl ProcessEvent(
			const RE::BSAnimationGraphEvent* a_event,
			RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override
		{
			if (a_event && g_cfg.logGraphEvents) {
				spdlog::info("graph event: {}", a_event->tag.c_str());
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	class InputSink final : public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static InputSink* GetSingleton()
		{
			static InputSink singleton;
			return std::addressof(singleton);
		}

		RE::BSEventNotifyControl ProcessEvent(
			RE::InputEvent* const* a_event,
			RE::BSTEventSource<RE::InputEvent*>*) override
		{
			if (!a_event) {
				return RE::BSEventNotifyControl::kContinue;
			}

			const auto* ue = RE::UserEvents::GetSingleton();

			for (auto* e = *a_event; e; e = e->next) {
				if (e->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
					continue;
				}
				const auto* btn = e->AsButtonEvent();
				if (!btn) {
					continue;
				}

				// СЃР»РµРґРёРј Р·Р° СЃРѕСЃС‚РѕСЏРЅРёРµРј РєР»Р°РІРёС€ РґРІРёР¶РµРЅРёСЏ
				if (ue) {
					const auto& evName = btn->QUserEvent();
					if (evName == ue->forward) {
						_forward = btn->IsPressed();
					} else if (evName == ue->back) {
						_back = btn->IsPressed();
					} else if (evName == ue->strafeLeft) {
						_left = btn->IsPressed();
					} else if (evName == ue->strafeRight) {
						_right = btn->IsPressed();
					}
				}

				if (btn->GetDevice() == RE::INPUT_DEVICE::kKeyboard &&
					btn->GetIDCode() == g_cfg.key &&
					btn->IsDown()) {
					TryDodge();
				}
			}

			return RE::BSEventNotifyControl::kContinue;
		}

	private:
		void TryDodge()
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui || ui->GameIsPaused() || ui->IsMenuOpen(RE::Console::MENU_NAME)) {
				return;
			}

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || !player->Is3DLoaded() || player->IsDead() || player->IsInRagdollState()) {
				return;
			}

			const auto* state = player->AsActorState();
			if (!state ||
				state->IsSwimming() ||
				state->IsFlying() ||
				state->IsBleedingOut() ||
				state->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal) {
				return;
			}

			if (_busy) {
				return;
			}

			const auto now = std::chrono::steady_clock::now();
			if (now - _lastDodge < std::chrono::milliseconds(g_cfg.cooldownMs)) {
				return;
			}

			// РЅР°РїСЂР°РІР»РµРЅРёРµ: WASD РѕС‚РЅРѕСЃРёС‚РµР»СЊРЅРѕ РєР°РјРµСЂС‹
			const float x = (_right ? 1.0f : 0.0f) - (_left ? 1.0f : 0.0f);
			const float y = (_forward ? 1.0f : 0.0f) - (_back ? 1.0f : 0.0f);

			// Р·Р°РїРѕРјРёРЅР°РµРј РєР°РјРµСЂСѓ Р”Рћ Р»СЋР±С‹С… РёР·РјРµРЅРµРЅРёР№
			auto* cam = RE::PlayerCamera::GetSingleton();
			_wasFirstPerson = cam && cam->IsInFirstPerson();
			_savedYaw = player->GetAngleZ();

			// С†РµР»РµРІРѕР№ РїРѕРІРѕСЂРѕС‚ РїРµСЂСЃРѕРЅР°Р¶Р°
			std::optional<float> targetYaw;
			if (g_cfg.rotateToInput && (x != 0.0f || y != 0.0f)) {
				if (const auto camYaw = GetCameraYaw()) {
					targetYaw = NormalizeAngle(*camYaw + std::atan2(x, y));
				}
			}

			_lastDodge = now;
			_busy = true;

			const bool needSneak = g_cfg.tempSneak && !player->IsSneaking();
			if (!needSneak) {
				// СѓР¶Рµ РІ СЃРєСЂС‹С‚РЅРѕСЃС‚Рё: РєР°С‚РёРјСЃСЏ СЃСЂР°Р·Сѓ
				DoRoll(targetYaw, false);
				return;
			}

			// 1) РІС…РѕРґРёРј РІ СЃРєСЂС‹С‚РЅРѕСЃС‚СЊ
			player->AsActorState()->actorState1.sneaking = 1;
			player->NotifyAnimationGraph(RE::BSFixedString(g_cfg.sneakStartEvent));
			spdlog::info("temporary sneak start");

			// 2) С‡РµСЂРµР· РїР°СЂСѓ РєР°РґСЂРѕРІ, РєРѕРіРґР° РіСЂР°С„ РїРµСЂРµС€С‘Р» РІ sneak-СЃРѕСЃС‚РѕСЏРЅРёРµ, РѕС‚РїСЂР°РІР»СЏРµРј РїРµСЂРµРєР°С‚
			RunLater(g_cfg.sneakEnterDelayMs, [this, targetYaw]() { DoRoll(targetYaw, true); });
		}

		void DoRoll(std::optional<float> a_yaw, bool a_leaveSneak)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || !player->Is3DLoaded()) {
				_busy = false;
				return;
			}

			if (a_yaw) {
				player->SetHeading(*a_yaw);
			}

			bool ok = false;
			for (const auto& ev : g_cfg.events) {
				ok = player->NotifyAnimationGraph(RE::BSFixedString(ev));
				spdlog::info("NotifyAnimationGraph(\"{}\") -> {}", ev, ok);
				if (ok) {
					break;
				}
			}

			if (!ok) {
				Finish(a_leaveSneak, false);
				return;
			}

			// 3) РїРѕСЃР»Рµ РїРµСЂРµРєР°С‚Р° РІРѕР·РІСЂР°С‰Р°РµРј РІСЃС‘ РєР°Рє Р±С‹Р»Рѕ
			RunLater(g_cfg.rollDurationMs, [this, a_leaveSneak]() { Finish(a_leaveSneak, true); });
		}

		void Finish(bool a_leaveSneak, bool a_rolled)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (player && player->Is3DLoaded()) {
				if (a_leaveSneak) {
					player->NotifyAnimationGraph(RE::BSFixedString(g_cfg.sneakStopEvent));
					if (auto* st = player->AsActorState()) {
						st->actorState1.sneaking = 0;
					}
					spdlog::info("temporary sneak end (rolled={})", a_rolled);
				}

				if (g_cfg.restoreCamera) {
					if (auto* cam = RE::PlayerCamera::GetSingleton()) {
						// РІРµСЂРЅСѓС‚СЊ РІРёРґ, РµСЃР»Рё РёРіСЂР° РµРіРѕ РїРµСЂРµРєР»СЋС‡РёР»Р°
						if (_wasFirstPerson && !cam->IsInFirstPerson()) {
							cam->ForceFirstPerson();
						} else if (!_wasFirstPerson && !cam->IsInThirdPerson()) {
							cam->ForceThirdPerson();
						}
						// РѕС‚ РїРµСЂРІРѕРіРѕ Р»РёС†Р° РїРѕРІРѕСЂРѕС‚ РїРµСЂСЃРѕРЅР°Р¶Р° = РїРѕРІРѕСЂРѕС‚ РєР°РјРµСЂС‹, РІРѕР·РІСЂР°С‰Р°РµРј РµРіРѕ
						if (_wasFirstPerson) {
							player->SetHeading(_savedYaw);
						}
					}
				}
			}
			_busy = false;
		}

		bool _forward{ false };
		bool _back{ false };
		bool _left{ false };
		bool _right{ false };
		std::chrono::steady_clock::time_point _lastDodge{};
		bool _busy{ false };
		bool _wasFirstPerson{ false };
		float _savedYaw{ 0.0f };
	};

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		if (a_msg->type != SKSE::MessagingInterface::kDataLoaded) {
			return;
		}

		if (auto* mgr = RE::BSInputDeviceManager::GetSingleton()) {
			mgr->AddEventSink(InputSink::GetSingleton());
			if (auto* pc = RE::PlayerCharacter::GetSingleton()) {
				pc->AddAnimationGraphEventSink(GraphSink::GetSingleton());
			}
			spdlog::info("Input sink registered. Key scancode={}, cooldown={} ms, rotate={}",
				g_cfg.key, g_cfg.cooldownMs, g_cfg.rotateToInput);
		} else {
			spdlog::error("BSInputDeviceManager is null, plugin inactive");
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SetupLog();
	g_cfg.Load();

	spdlog::info("VanillaDirectionalDodge loaded");

	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}
