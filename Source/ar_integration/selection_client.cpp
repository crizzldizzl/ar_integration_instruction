#include "selection_client.h"

bool U_selection_client::send_selection(const FString& object_id)
{
	if (!channel || !channel->channel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SelectionClient] Channel invalid. cannot send selection!"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[SelectionClient] Sending selection for object_id: %s"), *object_id);

	grpc::ClientContext ctx;
	generated::SelectionMessage msg;
	msg.set_object_id(TCHAR_TO_UTF8(*object_id));

	google::protobuf::Empty reply;
	auto status = stub->send_selection(&ctx, msg, &reply);

	if (status.ok())
	{
		UE_LOG(LogTemp, Log, TEXT("[SelectionClient] Successfully sent selection for %s"), *object_id);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SelectionClient] Failed to send selection for %s. gRPC Error: %d, %s"),
			*object_id,
			static_cast<int>(status.error_code()),
			*FString(status.error_message().c_str()));
	}

	return status.ok();
}
