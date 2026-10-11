// VanillaDirectionalDodge
// Р СњР В°Р В¶Р В°РЎвЂљР С‘Р Вµ Р С”Р В»Р В°Р Р†Р С‘РЎв‚¬Р С‘ -> Р С—Р С•Р Р†Р С•РЎР‚Р С•РЎвЂљ Р С—Р ВµРЎР‚РЎРѓР С•Р Р…Р В°Р В¶Р В° Р Р† РЎРѓРЎвЂљР С•РЎР‚Р С•Р Р…РЎС“ Р Т‘Р Р†Р С‘Р В¶Р ВµР Р…Р С‘РЎРЏ (WASD Р С•РЎвЂљР Р…Р С•РЎРѓР С‘РЎвЂљР ВµР В»РЎРЉР Р…Р С• Р С”Р В°Р СР ВµРЎР‚РЎвЂ№)
// -> Р Р†РЎР‚Р ВµР СР ВµР Р…Р Р…РЎвЂ№Р в„– Р Р†РЎвЂ¦Р С•Р Т‘ Р Р† РЎРѓР С”РЎР‚РЎвЂ№РЎвЂљР Р…Р С•РЎРѓРЎвЂљРЎРЉ -> Р Р†Р В°Р Р…Р С‘Р В»РЎРЉР Р…РЎвЂ№Р в„– Р С—Р ВµРЎР‚Р ВµР С”Р В°РЎвЂљ -> Р Р†Р С•Р В·Р Р†РЎР‚Р В°РЎвЂљ РЎРѓР С•РЎРѓРЎвЂљР С•РЎРЏР Р…Р С‘РЎРЏ Р С‘ Р С”Р В°Р СР ВµРЎР‚РЎвЂ№.

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <atomic>
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
		std::uint32_t cooldownMs = 800;  // Р С—Р В°РЎС“Р В·Р В° Р СР ВµР В¶Р Т‘РЎС“ Р С—Р ВµРЎР‚Р ВµР С”Р В°РЎвЂљР В°Р СР С‘
		bool rotateToInput = true;       // РЎР‚Р В°Р В·Р Р†Р С•РЎР‚Р В°РЎвЂЎР С‘Р Р†Р В°РЎвЂљРЎРЉ Р С—Р ВµРЎР‚РЎРѓР С•Р Р…Р В°Р В¶Р В° Р С—Р С• WASD
		std::vector<std::string> events{ "SneakSprintStartRoll" };

		// Р СџР ВµРЎР‚Р ВµР С”Р В°РЎвЂљ Р С—РЎР‚Р С‘Р Р…Р С‘Р СР В°Р ВµРЎвЂљРЎРѓРЎРЏ Р С–РЎР‚Р В°РЎвЂћР С•Р С Р В°Р Р…Р С‘Р СР В°РЎвЂ Р С‘Р в„– РЎвЂљР С•Р В»РЎРЉР С”Р С• Р Р† РЎРѓР С•РЎРѓРЎвЂљР С•РЎРЏР Р…Р С‘Р С‘ РЎРѓР С”РЎР‚РЎвЂ№РЎвЂљР Р…Р С•РЎРѓРЎвЂљР С‘,
		// Р С—Р С•РЎРЊРЎвЂљР С•Р СРЎС“ Р Р†Р Р…Р Вµ РЎРѓР С”РЎР‚РЎвЂ№РЎвЂљР Р…Р С•РЎРѓРЎвЂљР С‘ Р СРЎвЂ№ Р Р†РЎР‚Р ВµР СР ВµР Р…Р Р…Р С• Р Р†РЎвЂ¦Р С•Р Т‘Р С‘Р С Р Р† Р Р…Р ВµРЎвЂ Р С‘ Р С—Р С•РЎвЂљР С•Р С Р Р†РЎвЂ№РЎвЂ¦Р С•Р Т‘Р С‘Р С.
		bool tempSneak = true;
		std::uint32_t sneakEnterDelayMs = 120;  // Р С•Р В¶Р С‘Р Т‘Р В°Р Р…Р С‘Р Вµ, Р С—Р С•Р С”Р В° Р С–РЎР‚Р В°РЎвЂћ Р С—Р ВµРЎР‚Р ВµР в„–Р Т‘РЎвЂРЎвЂљ Р Р† sneak-РЎРѓР С•РЎРѓРЎвЂљР С•РЎРЏР Р…Р С‘Р Вµ
		std::uint32_t rollDurationMs = 1000;    // РЎРѓР С”Р С•Р В»РЎРЉР С”Р С• Р В¶Р Т‘Р В°РЎвЂљРЎРЉ Р Т‘Р С• Р Р†РЎвЂ№РЎвЂ¦Р С•Р Т‘Р В° Р С‘Р В· РЎРѓР С”РЎР‚РЎвЂ№РЎвЂљР Р…Р С•РЎРѓРЎвЂљР С‘
		bool restoreCamera = true;              // Р Р†Р ВµРЎР‚Р Р…РЎС“РЎвЂљРЎРЉ Р Р†Р С‘Р Т‘ (1-Р Вµ/3-Р Вµ Р В»Р С‘РЎвЂ Р С•) Р С‘ Р С—Р С•Р Р†Р С•РЎР‚Р С•РЎвЂљ Р С”Р В°Р СР ВµРЎР‚РЎвЂ№
		bool logGraphEvents = false;            // Р С—Р С‘РЎРѓР В°РЎвЂљРЎРЉ Р Р† Р В»Р С•Р С– РЎРѓР С•Р В±РЎвЂ№РЎвЂљР С‘РЎРЏ Р С–РЎР‚Р В°РЎвЂћР В° (Р Т‘Р В»РЎРЏ Р С•РЎвЂљР В»Р В°Р Т‘Р С”Р С‘)
		std::string sneakStartEvent = "SneakStart";
		std::string sneakStopEvent = "SneakStop";

		void Load()
		{
			key = ::GetPrivateProfileIntA("General", "Key", static_cast<int>(key), kIniPath);
			cooldownMs = ::GetPrivateProfileIntA("General", "CooldownMs", static_cast<int>(cooldownMs), kIniPath);
			rotateToInput = ::GetPrivateProfileIntA("General", "RotateToInput", 1, kIniPath) != 0;
			// Keep accidental INI values from creating instant repeats or very long lockouts.
			cooldownMs = std::clamp(cooldownMs, 100u, 5000u);

			tempSneak = ::GetPrivateProfileIntA("General", "TemporarySneak", 1, kIniPath) != 0;
			sneakEnterDelayMs = ::GetPrivateProfileIntA("General", "SneakEnterDelayMs", static_cast<int>(sneakEnterDelayMs), kIniPath);
			sneakEnterDelayMs = std::clamp(sneakEnterDelayMs, 0u, 500u);
			rollDurationMs = ::GetPrivateProfileIntA("General", "RollDurationMs", static_cast<int>(rollDurationMs), kIniPath);
			rollDurationMs = std::clamp(rollDurationMs, 300u, 2500u);
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
		// Р С”Р В°Р СР ВµРЎР‚Р В° Skyrim РЎРѓР СР С•РЎвЂљРЎР‚Р С‘РЎвЂљ Р Р†Р Т‘Р С•Р В»РЎРЉ Р В»Р С•Р С”Р В°Р В»РЎРЉР Р…Р С•Р в„– Р С•РЎРѓР С‘ Y
		const auto& r = cam->world.rotate;
		return std::atan2(r.entry[0][1], r.entry[1][1]);
	}

	float NormalizeAngle(float a)
	{
		constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
		a = std::fmod(a, twoPi);
		return a < 0.0f ? a + twoPi : a;
	}

	// Р Р†РЎвЂ№Р С—Р С•Р В»Р Р…Р С‘РЎвЂљРЎРЉ Р Т‘Р ВµР в„–РЎРѓРЎвЂљР Р†Р С‘Р Вµ Р Р† Р С–Р В»Р В°Р Р†Р Р…Р С•Р С Р С—Р С•РЎвЂљР С•Р С”Р Вµ Р С‘Р С–РЎР‚РЎвЂ№ РЎвЂЎР ВµРЎР‚Р ВµР В· delayMs
	template <class F>
	void RunLater(std::uint32_t delayMs, F&& fn)
	{
		std::thread([delayMs, fn = std::forward<F>(fn)]() mutable {
			std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
			SKSE::GetTaskInterface()->AddTask(std::move(fn));
		}).detach();
	}

	// ---- Р С•РЎвЂљР В»Р В°Р Т‘Р С•РЎвЂЎР Р…РЎвЂ№Р в„– Р С—РЎР‚Р С‘РЎвЂР СР Р…Р С‘Р С” РЎРѓР С•Р В±РЎвЂ№РЎвЂљР С‘Р в„– Р С–РЎР‚Р В°РЎвЂћР В° Р В°Р Р…Р С‘Р СР В°РЎвЂ Р С‘Р в„– Р С‘Р С–РЎР‚Р С•Р С”Р В°
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

				// РЎРѓР В»Р ВµР Т‘Р С‘Р С Р В·Р В° РЎРѓР С•РЎРѓРЎвЂљР С•РЎРЏР Р…Р С‘Р ВµР С Р С”Р В»Р В°Р Р†Р С‘РЎв‚¬ Р Т‘Р Р†Р С‘Р В¶Р ВµР Р…Р С‘РЎРЏ
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
					btn->IsPressed()) {
					TryDodge();
				}
			}

			return RE::BSEventNotifyControl::kContinue;
		}

	private:
		void TryDodge()
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui || ui->GameIsPaused() || ui->IsMenuOpen(RE::Console::MENU_NAME) ||
				ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME) || ui->IsMenuOpen(RE::MagicMenu::MENU_NAME) ||
				ui->IsMenuOpen(RE::MapMenu::MENU_NAME) || ui->IsMenuOpen(RE::JournalMenu::MENU_NAME)) {
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

			// Р Р…Р В°Р С—РЎР‚Р В°Р Р†Р В»Р ВµР Р…Р С‘Р Вµ: WASD Р С•РЎвЂљР Р…Р С•РЎРѓР С‘РЎвЂљР ВµР В»РЎРЉР Р…Р С• Р С”Р В°Р СР ВµРЎР‚РЎвЂ№
			const float x = (_right ? 1.0f : 0.0f) - (_left ? 1.0f : 0.0f);
			const float y = (_forward ? 1.0f : 0.0f) - (_back ? 1.0f : 0.0f);

			// Р В·Р В°Р С—Р С•Р СР С‘Р Р…Р В°Р ВµР С Р С”Р В°Р СР ВµРЎР‚РЎС“ Р вЂќР С› Р В»РЎР‹Р В±РЎвЂ№РЎвЂ¦ Р С‘Р В·Р СР ВµР Р…Р ВµР Р…Р С‘Р в„–
			auto* cam = RE::PlayerCamera::GetSingleton();
			_wasFirstPerson = cam && cam->IsInFirstPerson();
			_savedYaw = player->GetAngleZ();

			// РЎвЂ Р ВµР В»Р ВµР Р†Р С•Р в„– Р С—Р С•Р Р†Р С•РЎР‚Р С•РЎвЂљ Р С—Р ВµРЎР‚РЎРѓР С•Р Р…Р В°Р В¶Р В°
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
				// РЎС“Р В¶Р Вµ Р Р† РЎРѓР С”РЎР‚РЎвЂ№РЎвЂљР Р…Р С•РЎРѓРЎвЂљР С‘: Р С”Р В°РЎвЂљР С‘Р СРЎРѓРЎРЏ РЎРѓРЎР‚Р В°Р В·РЎС“
				DoRoll(targetYaw, false);
				return;
			}

			// 1) Р Р†РЎвЂ¦Р С•Р Т‘Р С‘Р С Р Р† РЎРѓР С”РЎР‚РЎвЂ№РЎвЂљР Р…Р С•РЎРѓРЎвЂљРЎРЉ
			player->AsActorState()->actorState1.sneaking = 1;
			player->NotifyAnimationGraph(RE::BSFixedString(g_cfg.sneakStartEvent));
			spdlog::info("temporary sneak start");

			// 2) РЎвЂЎР ВµРЎР‚Р ВµР В· Р С—Р В°РЎР‚РЎС“ Р С”Р В°Р Т‘РЎР‚Р С•Р Р†, Р С”Р С•Р С–Р Т‘Р В° Р С–РЎР‚Р В°РЎвЂћ Р С—Р ВµРЎР‚Р ВµРЎв‚¬РЎвЂР В» Р Р† sneak-РЎРѓР С•РЎРѓРЎвЂљР С•РЎРЏР Р…Р С‘Р Вµ, Р С•РЎвЂљР С—РЎР‚Р В°Р Р†Р В»РЎРЏР ВµР С Р С—Р ВµРЎР‚Р ВµР С”Р В°РЎвЂљ
			RunLater(g_cfg.sneakEnterDelayMs, [this, targetYaw]() { DoRoll(targetYaw, true); });
		}

		void DoRoll(std::optional<float> a_yaw, bool a_leaveSneak)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || !player->Is3DLoaded() || player->IsDead() || player->IsInRagdollState()) {
				if (a_leaveSneak) {
					Finish(true, false);
				} else {
					_busy = false;
				}
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

			// 3) Р С—Р С•РЎРѓР В»Р Вµ Р С—Р ВµРЎР‚Р ВµР С”Р В°РЎвЂљР В° Р Р†Р С•Р В·Р Р†РЎР‚Р В°РЎвЂ°Р В°Р ВµР С Р Р†РЎРѓРЎвЂ Р С”Р В°Р С” Р В±РЎвЂ№Р В»Р С•
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
						// Р Р†Р ВµРЎР‚Р Р…РЎС“РЎвЂљРЎРЉ Р Р†Р С‘Р Т‘, Р ВµРЎРѓР В»Р С‘ Р С‘Р С–РЎР‚Р В° Р ВµР С–Р С• Р С—Р ВµРЎР‚Р ВµР С”Р В»РЎР‹РЎвЂЎР С‘Р В»Р В°
						if (_wasFirstPerson && !cam->IsInFirstPerson()) {
							cam->ForceFirstPerson();
						} else if (!_wasFirstPerson && !cam->IsInThirdPerson()) {
							cam->ForceThirdPerson();
						}
						// Р С•РЎвЂљ Р С—Р ВµРЎР‚Р Р†Р С•Р С–Р С• Р В»Р С‘РЎвЂ Р В° Р С—Р С•Р Р†Р С•РЎР‚Р С•РЎвЂљ Р С—Р ВµРЎР‚РЎРѓР С•Р Р…Р В°Р В¶Р В° = Р С—Р С•Р Р†Р С•РЎР‚Р С•РЎвЂљ Р С”Р В°Р СР ВµРЎР‚РЎвЂ№, Р Р†Р С•Р В·Р Р†РЎР‚Р В°РЎвЂ°Р В°Р ВµР С Р ВµР С–Р С•
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
		std::atomic_bool _busy{ false };
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
