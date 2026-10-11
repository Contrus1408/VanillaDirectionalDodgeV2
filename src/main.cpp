// VanillaDirectionalDodge
// Нажатие клавиши -> поворот персонажа в сторону движения (WASD относительно камеры)
// -> отправка ванильного анимационного события переката (перки/бег/присед не нужны).

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <spdlog/sinks/basic_file_sink.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	constexpr auto kIniPath = "Data\\SKSE\\Plugins\\VanillaDirectionalDodge.ini";

	// ---------------------------------------------------------------- config
	struct Config
	{
		std::uint32_t key = 47;          // DirectInput scancode, 47 = V
		std::uint32_t cooldownMs = 800;  // пауза между перекатами
		bool rotateToInput = true;       // разворачивать персонажа по WASD
		std::vector<std::string> events{ "SneakSprintStartRoll" };

		void Load()
		{
			key = ::GetPrivateProfileIntA("General", "Key", static_cast<int>(key), kIniPath);
			cooldownMs = ::GetPrivateProfileIntA("General", "CooldownMs", static_cast<int>(cooldownMs), kIniPath);
			rotateToInput = ::GetPrivateProfileIntA("General", "RotateToInput", 1, kIniPath) != 0;

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
		// камера Skyrim смотрит вдоль локальной оси Y
		const auto& r = cam->world.rotate;
		return std::atan2(r.entry[0][1], r.entry[1][1]);
	}

	float NormalizeAngle(float a)
	{
		constexpr float twoPi = 2.0f * std::numbers::pi_v<float>;
		a = std::fmod(a, twoPi);
		return a < 0.0f ? a + twoPi : a;
	}

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

								// следим за состоянием клавиш движения
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

			const auto now = std::chrono::steady_clock::now();
			if (now - _lastDodge < std::chrono::milliseconds(g_cfg.cooldownMs)) {
				return;
			}

			// направление: WASD относительно камеры
			const float x = (_right ? 1.0f : 0.0f) - (_left ? 1.0f : 0.0f);
			const float y = (_forward ? 1.0f : 0.0f) - (_back ? 1.0f : 0.0f);

			if (g_cfg.rotateToInput && (x != 0.0f || y != 0.0f)) {
				if (const auto camYaw = GetCameraYaw()) {
					player->data.angle.z = NormalizeAngle(*camYaw + std::atan2(x, y));
				}
			}

			// ванильный перекат: пробуем события из ini по очереди
			for (const auto& ev : g_cfg.events) {
				const bool ok = player->NotifyAnimationGraph(RE::BSFixedString(ev));
				spdlog::info("NotifyAnimationGraph(\"{}\") -> {}", ev, ok);
				if (ok) {
					_lastDodge = now;
					return;
				}
			}
		}

		bool _forward{ false };
		bool _back{ false };
		bool _left{ false };
		bool _right{ false };
		std::chrono::steady_clock::time_point _lastDodge{};
	};

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		if (a_msg->type != SKSE::MessagingInterface::kDataLoaded) {
			return;
		}

		if (auto* mgr = RE::BSInputDeviceManager::GetSingleton()) {
			mgr->AddEventSink(InputSink::GetSingleton());
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
