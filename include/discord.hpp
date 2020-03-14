#ifndef __DISCORD_HPP
#define __DISCORD_HPP

#include <cstdlib>
#include <cstdint>
#include <discord_game_sdk.h>

class DiscordSDK {
private:
	struct IDiscordUserEvents user_events;
	struct IDiscordActivityEvents activity_events;
public:
	struct IDiscordCore* core = NULL;
	struct IDiscordUserManager* users = NULL;
	struct IDiscordActivityManager* activities = NULL;
	struct DiscordActivity rpc;

	// Callbacks
	void (*OnError)(void* data, enum EDiscordResult result) = NULL;
	void (*OnRpcUpdate)(void* data, enum EDiscordResult result) = NULL;
	void (*OnUserUpdate)(void* data) = NULL;
	void (*OnJoinRequest)(void* data, struct DiscordUser* user) = NULL;
	void (*OnJoin)(void* data, const char* secret) = NULL;
	void (*OnSpectate)(void* data, const char* secret) = NULL;
	void (*OnInvite)(void* data, enum EDiscordActivityActionType type, struct DiscordUser* user, struct DiscordActivity* activity) = NULL;

	void Init(int64_t id) {
		memset(&user_events, 0, sizeof(user_events));
		user_events.on_current_user_update = OnUserUpdate;

		memset(&activity_events, 0, sizeof(activity_events));
		activity_events.on_activity_join_request = OnJoinRequest;
		activity_events.on_activity_join = OnJoin;
		activity_events.on_activity_spectate = OnSpectate;
		activity_events.on_activity_invite = OnInvite;

		memset(&rpc, 0, sizeof(rpc));

		struct DiscordCreateParams params;
		DiscordCreateParamsSetDefault(&params);
		params.client_id = id;
		params.flags = DiscordCreateFlags_Default;
		params.event_data = this;
		params.user_events = &user_events;
		params.activity_events = &activity_events;
		enum EDiscordResult res = DiscordCreate(DISCORD_VERSION, &params, &core);
		if(res != DiscordResult_Ok) {
			OnError((void*)this, res);
		} else {
			this->users = core->get_user_manager(core);
			this->activities = core->get_activity_manager(core);
		}
	}

	~DiscordSDK() {
		if(core != NULL) core->destroy(core);
	}

	void RunTasks() {
		if(core != NULL) {
			enum EDiscordResult res = core->run_callbacks(core);
			if(res != DiscordResult_Ok) {
				OnError((void*)this, res);
			}
		}
	}

	void UpdateRPC() {
		if(activities != NULL) activities->update_activity(activities, &rpc, this, OnRpcUpdate);
	}

	void ClearRPC() {
		if(activities != NULL) activities->clear_activity(activities, this, OnRpcUpdate);
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
};

#endif
