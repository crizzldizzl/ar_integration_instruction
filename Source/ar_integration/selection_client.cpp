#include "selection_client.h"

bool U_selection_client::send_selection(const FString& mesh_id, int32 pn_id, assignment_type assignment)
{
	if (!channel || !channel->channel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[selection_client] Channel invalid. Cannot send selection!"));
		return false;
	}

	const int32 assignment_raw = static_cast<int32>(assignment);
	UE_LOG(LogTemp, Log, TEXT("[selection_client] Sending selection %s (PN-ID: %d, Assignment: %d)"), *mesh_id, pn_id, assignment_raw);

	grpc::ClientContext ctx;
	generated::selection_message msg;

	msg.set_pn_id(pn_id);
	msg.set_object_id(TCHAR_TO_UTF8(*mesh_id));
	msg.set_assignment(static_cast<generated::assignment_type>(assignment_raw));

	google::protobuf::Empty reply;
	auto status = stub_->send_selection(&ctx, msg, &reply);

	if (status.ok())
	{
		UE_LOG(LogTemp, Log, TEXT("[selection_client] Successfully sent selection: %s (PN-ID: %d)"), *mesh_id, pn_id);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[selection_client] Failed to send selection. Error code: %d, message: %s"), static_cast<int>(status.error_code()), *FString(status.error_message().c_str()));
	}

	return status.ok();
}

bool U_selection_client::request_scenario(scenario_type& scenario)
{
    scenario = scenario_type::MIXED;

    if (!channel || !channel->channel)
    {
        UE_LOG(LogTemp, Warning, TEXT("[selection_client] Channel invalid. Cannot request scenario!"));
        return false;
    }

    grpc::ClientContext ctx;
    google::protobuf::Empty req;
    generated::scenario_request resp;

    auto status = stub_->get_scenario(&ctx, req, &resp);
    if (!status.ok())
    {
        UE_LOG(LogTemp, Error, TEXT("[selection_client] Failed to get scenario. Error code: %d, message: %s"), static_cast<int>(status.error_code()), *FString(status.error_message().c_str()));
        return false;
    }

    switch (resp.scenario())
    {
    case 0: 
        scenario = scenario_type::DELEGATE_ONLY; 
        break;

    case 1: 
        scenario = scenario_type::RESERVE_ONLY;  
        break;

    case 2: 
        scenario = scenario_type::MIXED;
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("[selection_client] Unknown scenario value %d. Falling back to MIXED."),resp.scenario());
        scenario = scenario_type::MIXED;
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("[selection_client] Scenario updated to %d"), static_cast<int32>(scenario));
    return true;
}
