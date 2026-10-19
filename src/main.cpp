// Click Assist
//
// Lets a jump click land a few frames early and still count. When you press
// jump and nothing can use the click yet, the press is remembered for a short
// window. If an orb/ring or the ground becomes usable inside that window and
// the click is still "in range", the click is replayed on that exact frame.
//
// It only ever acts on a real press: no press, no assist.

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include <cstdint>
#include <unordered_set>

using namespace geode::prelude;

namespace {
	// Simulated time, in seconds, summed from the dt of every physics tick.
	// Only differences between two readings are ever used.
	double g_clock = 0.0;

	// One "frame" here is one physics step at 1x speed (240 per second).
	constexpr double kFrame = 1.0 / 240.0;
	// Tolerance so a click exactly N frames old still counts as N frames old.
	constexpr double kSlack = 0.0001;

	bool settingOn(char const* key) {
		return Mod::get()->getSettingValue<bool>(key);
	}

	double windowSeconds() {
		return static_cast<double>(Mod::get()->getSettingValue<int64_t>("window")) * kFrame;
	}
}

class $modify(AssistPlayer, PlayerObject) {
	struct Fields {
		bool pending = false;       // a real press is waiting for something to land on
		bool userHolding = false;   // is the player physically holding jump right now
		bool synthetic = false;     // true while we are issuing our own press/release
		bool releaseNext = false;   // we fired a tap and still owe it a release
		bool wasOnGround = false;
		double clickTime = 0.0;
		std::unordered_set<int> prevRings;
	};

	void clearAssist() {
		m_fields->pending = false;
		m_fields->releaseNext = false;
	}

	bool pushButton(PlayerButton button) {
		if (button == PlayerButton::Jump && !m_fields->synthetic && m_gameLayer) {
			m_fields->userHolding = true;
			if (settingOn("enabled")) {
				m_fields->pending = true;
				m_fields->clickTime = g_clock;
			}
		}
		return PlayerObject::pushButton(button);
	}

	bool releaseButton(PlayerButton button) {
		if (button == PlayerButton::Jump && !m_fields->synthetic) {
			m_fields->userHolding = false;
		}
		return PlayerObject::releaseButton(button);
	}

	void releaseAllButtons() {
		m_fields->userHolding = false;
		this->clearAssist();
		PlayerObject::releaseAllButtons();
	}

	void playerDestroyed(bool noEffects) {
		this->clearAssist();
		PlayerObject::playerDestroyed(noEffects);
	}

	// Called once per collision pass, right after the game has worked out
	// what the player is touching this tick.
	void assistTick() {
		auto f = m_fields.self();

		if (!m_gameLayer || m_isDead) {
			f->pending = false;
			return;
		}

		auto self = static_cast<PlayerObject*>(this);

		// Finish a tap that we started on an earlier tick.
		if (f->releaseNext) {
			f->releaseNext = false;
			if (!f->userHolding) {
				f->synthetic = true;
				self->releaseButton(PlayerButton::Jump);
				f->synthetic = false;
			}
		}

		// Work out what became usable this tick. Edges only, so something we
		// are already standing on or overlapping does not fire again.
		bool newRing = false;
		for (int id : m_touchedRings) {
			if (!f->prevRings.count(id)) {
				newRing = true;
				break;
			}
		}
		f->prevRings = std::unordered_set<int>(m_touchedRings.begin(), m_touchedRings.end());

		bool landed = m_isOnGround && !f->wasOnGround;
		f->wasOnGround = m_isOnGround;

		if (!f->pending) return;

		double age = g_clock - f->clickTime;
		if (age > windowSeconds() + kSlack) {
			f->pending = false;   // too early, the click expires unused
			return;
		}
		// Same tick as the press: the game already handled it normally.
		if (age < kSlack) return;

		bool fire = (settingOn("rings") && newRing)
			|| (settingOn("ground") && landed && !this->isFlying());
		if (!fire) return;

		// Replay the click on this exact tick.
		f->pending = false;
		f->synthetic = true;
		self->releaseButton(PlayerButton::Jump);
		self->pushButton(PlayerButton::Jump);
		f->synthetic = false;

		// If the player had already let go, make it a tap rather than a hold.
		if (!f->userHolding) f->releaseNext = true;
	}
};

class $modify(AssistLayer, GJBaseGameLayer) {
	void processCommands(float dt, bool isHalfTick, bool isLastTick) {
		g_clock += dt;
		GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
	}

	int checkCollisions(PlayerObject* player, float dt, bool ignoreDamage) {
		int ret = GJBaseGameLayer::checkCollisions(player, dt, ignoreDamage);
		if (player && (player == m_player1 || player == m_player2) && settingOn("enabled")) {
			static_cast<AssistPlayer*>(player)->assistTick();
		}
		return ret;
	}
};
