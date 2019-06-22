#ifndef __DISCORD_RPC_HPP
#define __DISCORD_RPC_HPP

#include <discord_rpc.h>
#include <cstring>
#include <iostream>

#include "../include/game.hpp"

namespace DiscordRPC {
	// Variables needed by UpdatePresence()
	std::string State = "";
	std::string Details = "";

	int64_t StartTimestamp = 0;
	int64_t EndTimestamp = 0;

	std::string SmallImageKey;
	std::string LargeImageKey;
	std::string SmallImageText;
	std::string LargeImageText;

	std::string PartyId;
	int PartySize = 0;
	int PartyMax = 0;

	std::string MatchSecret;
	std::string JoinSecret;
	std::string SpectateSecret;

	int64_t Instance = 0;

	// Callbacks
	void OnReady(const DiscordUser* user);
	void OnDisconnect(int errcode, const char* message);
	void OnError(int errcode, const char* message);
	void OnJoin(const char* secret);
	void OnJoinRequest(const DiscordUser* user);
	void OnSpectate(const char* secret);

	void Init(const char* discordAppID) {
		DiscordEventHandlers handlers;
		memset(&handlers, 0, sizeof(handlers));
		handlers.ready = OnReady;
		handlers.disconnected = OnDisconnect;
		handlers.errored = OnError;
		handlers.joinGame = OnJoin;
		handlers.spectateGame = OnSpectate;
		handlers.joinRequest = OnJoinRequest;
		Discord_Initialize(discordAppID, &handlers, 1, NULL);
	}

	void UpdatePresence() {
		DiscordRichPresence discordPresence;
		memset(&discordPresence, 0, sizeof(discordPresence));

		discordPresence.state = State.c_str();
		discordPresence.details = Details.c_str();

		discordPresence.startTimestamp = StartTimestamp;
		discordPresence.endTimestamp = EndTimestamp;

		discordPresence.largeImageKey = LargeImageKey.c_str();
		discordPresence.largeImageText = LargeImageText.c_str();
		discordPresence.smallImageKey = SmallImageKey.c_str();
		discordPresence.smallImageText = SmallImageText.c_str();

		discordPresence.partyId = PartyId.c_str();
		discordPresence.partySize = PartySize;
		discordPresence.partyMax = PartyMax;

		discordPresence.matchSecret = MatchSecret.c_str();
		discordPresence.joinSecret = JoinSecret.c_str();
		discordPresence.spectateSecret = SpectateSecret.c_str();

		discordPresence.instance = Instance;

		Discord_UpdatePresence(&discordPresence);
	}

	void RunTasks() {
		#ifdef DISCORD_DISABLE_IO_THREAD
			Discord_UpdateConnection();
		#endif
		Discord_RunCallbacks();
	}

	void Quit() {
		Discord_Shutdown();
	}
}

#endif
