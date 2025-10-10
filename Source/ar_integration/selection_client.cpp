#include "selection_client.h"

bool U_selection_client::send_selection(const FString& mesh_id, int32 pn_id)
{
	if (!channel || !channel->channel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SelectionClient] Channel invalid. Cannot send selection!"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[SelectionClient] Sending selection for mesh_id: %s with PN-ID: %d"), *mesh_id, pn_id);

	grpc::ClientContext ctx;
	generated::SelectionMessage msg;

	msg.set_pn_id(pn_id);
	msg.set_object_id(TCHAR_TO_UTF8(*mesh_id));

	google::protobuf::Empty reply;
	auto status = stub->send_selection(&ctx, msg, &reply);

	if (status.ok())
	{
		UE_LOG(LogTemp, Log, TEXT("[SelectionClient] Successfully sent selection: %s (PN-ID: %d)"), *mesh_id, pn_id);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SelectionClient] Failed to send selection. Error code: %d, message: %s"),
			static_cast<int>(status.error_code()),
			*FString(status.error_message().c_str()));
	}

	return status.ok();
}
