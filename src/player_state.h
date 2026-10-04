#pragma once

enum class PlayerState
{
	NotConnected, // Disconnected / in main menu
	Connecting, // Waiting to join (class selection screen / spawn wait)
	OnFoot, // On foot, actively playing
	InVehicle, // Driver or passenger in a vehicle
	Spectating, // Spectating another player
	Wasted, // Player is dead / wasted
	Paused, // Game window lost focus (AFK / tabbed out)
	InMenu, // SAMP pause menu is open
};

inline const char* PlayerStateLabel(PlayerState s)
{
	switch (s)
	{
	case PlayerState::Connecting:
		return "Connecting...";
	case PlayerState::OnFoot:
		return "On foot";
	case PlayerState::InVehicle:
		return "In a vehicle";
	case PlayerState::Spectating:
		return "Spectating";
	case PlayerState::Wasted:
		return "Wasted";
	case PlayerState::Paused:
		return "Paused";
	case PlayerState::InMenu:
		return "In menu";
	case PlayerState::NotConnected:
	default:
		return "Not connected";
	}
}
