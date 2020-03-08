#ifndef __DISCORD_HPP
#define __DISCORD_HPP

#include <cstdlib>
#include <cstdint>
#include <discord_game_sdk.h>

namespace DiscordSDK {
	namespace {
		struct IDiscordUserEvents user_events;
		struct IDiscordActivityEvents activity_events;
	}

	struct Application {
		struct IDiscordCore* core;
		struct IDiscordUserManager* users;
		struct IDiscordActivityManager* activities;
	} App;
	struct DiscordActivity RPC;

	// Callbacks
	void OnError(void* data, enum EDiscordResult result);
	void OnRpcUpdate(void* data, enum EDiscordResult result);
	void OnUserUpdate(void* data);
	void OnJoinRequest(void* data, struct DiscordUser* user);
	void OnJoin(void* data, const char* secret);
	void OnSpectate(void* data, const char* secret);
	void OnInvite(void* data, enum EDiscordActivityActionType type, struct DiscordUser* user, struct DiscordActivity* activity);

	enum EDiscordResult Init(int64_t id) {
		memset(&user_events, 0, sizeof(user_events));
		user_events.on_current_user_update = OnUserUpdate;

		memset(&activity_events, 0, sizeof(activity_events));
		activity_events.on_activity_join_request = OnJoinRequest;
		activity_events.on_activity_join = OnJoin;
		activity_events.on_activity_spectate = OnSpectate;
		activity_events.on_activity_invite = OnInvite;

		memset(&App, 0, sizeof(App));
		memset(&RPC, 0, sizeof(RPC));

		struct DiscordCreateParams params;
		DiscordCreateParamsSetDefault(&params);
		params.client_id = id;
		params.flags = DiscordCreateFlags_Default;
		params.event_data = &App;
		params.user_events = &user_events;
		params.activity_events = &activity_events;
		enum EDiscordResult res = DiscordCreate(DISCORD_VERSION, &params, &App.core);
		if(res != DiscordResult_Ok) {
			OnError((void*)&App, res);
		} else {
			App.users = App.core->get_user_manager(App.core);
			App.activities = App.core->get_activity_manager(App.core);
		}
		return res;
	}

	void Destroy() {
		App.core->destroy(App.core);
	}

	enum EDiscordResult RunTasks() {
		enum EDiscordResult res = App.core->run_callbacks(App.core);
		if(res != DiscordResult_Ok) {
			OnError((void*)&App, res);
		}
		return res;
	}

	const char* GetResultStr(enum EDiscordResult res) {
		switch(res) {
			case DiscordResult_Ok:
				return "Ok";
			case DiscordResult_ServiceUnavailable:
				return "ServiceUnavailable";
			case DiscordResult_InvalidVersion:
				return "InvalidVersion";
			case DiscordResult_LockFailed:
				return "LockFailed";
			case DiscordResult_InternalError:
				return "InternalError";
			case DiscordResult_InvalidPayload:
				return "InvalidPayload";
			case DiscordResult_InvalidCommand:
				return "InvalidCommand";
			case DiscordResult_InvalidPermissions:
				return "InvalidPermissions";
			case DiscordResult_NotFetched:
				return "NotFetched";
			case DiscordResult_NotFound:
				return "NotFound";
			case DiscordResult_Conflict:
				return "Conflict";
			case DiscordResult_InvalidSecret:
				return "InvalidSecret";
			case DiscordResult_InvalidJoinSecret:
				return "InvalidJoinSecret";
			case DiscordResult_NoEligibleActivity:
				return "NoEligibleActivity";
			case DiscordResult_InvalidInvite:
				return "InvalidInvite";
			case DiscordResult_NotAuthenticated:
				return "NotAuthenticated";
			case DiscordResult_InvalidAccessToken:
				return "InvalidAccessToken";
			case DiscordResult_ApplicationMismatch:
				return "ApplicationMismatch";
			case DiscordResult_InvalidDataUrl:
				return "InvalidDataUrl";
			case DiscordResult_InvalidBase64:
				return "InvalidBase64";
			case DiscordResult_NotFiltered:
				return "NotFiltered";
			case DiscordResult_LobbyFull:
				return "LobbyFull";
			case DiscordResult_InvalidLobbySecret:
				return "InvalidLobbySecret";
			case DiscordResult_InvalidFilename:
				return "InvalidFilename";
			case DiscordResult_InvalidFileSize:
				return "InvalidFileSize";
			case DiscordResult_InvalidEntitlement:
				return "InvalidEntitlement";
			case DiscordResult_NotInstalled:
				return "NotInstalled";
			case DiscordResult_NotRunning:
				return "NotRunning";
			case DiscordResult_InsufficientBuffer:
				return "InsufficientBuffer";
			case DiscordResult_PurchaseCanceled:
				return "PurchaseCanceled";
			case DiscordResult_InvalidGuild:
				return "InvalidGuild";
			case DiscordResult_InvalidEvent:
				return "InvalidEvent";
			case DiscordResult_InvalidChannel:
				return "InvalidChannel";
			case DiscordResult_InvalidOrigin:
				return "InvalidOrigin";
			case DiscordResult_RateLimited:
				return "RateLimited";
			case DiscordResult_OAuth2Error:
				return "OAuth2Error";
			case DiscordResult_SelectChannelTimeout:
				return "SelectChannelTimeout";
			case DiscordResult_GetGuildTimeout:
				return "GetGuildTimeout";
			case DiscordResult_SelectVoiceForceRequired:
				return "SelectVoiceForceRequired";
			case DiscordResult_CaptureShortcutAlreadyListening:
				return "CaptureShortcutAlreadyListening";
			case DiscordResult_UnauthorizedForAchievement:
				return "UnauthorizedForAchievement";
			case DiscordResult_InvalidGiftCode:
				return "InvalidGiftCode";
			case DiscordResult_PurchaseError:
				return "PurchaseError";
			case DiscordResult_TransactionAborted:
				return "TransactionAborted";
		}
		return "";
	}

	void UpdateRPC() {
		App.activities->update_activity(App.activities, &RPC, &App, OnRpcUpdate);
	}

	void ClearRPC() {
		App.activities->clear_activity(App.activities, &App, OnRpcUpdate);
	}
}

#endif
