#pragma once

#include "EngineMinimal.h"
#include "UObject/Object.h"

#include "grpc_wrapper.h"
#include "grpc_channel.h"
#include "base_client.h"

#include "grpc_include_begin.h"
#include "selection.grpc.pb.h"
#include "grpc_include_end.h"

#include "selection_client.generated.h"

/**
 * @class U_selection_client
 * client for sending selected mesh-object IDs to the server
 */
UCLASS()
class U_selection_client : public UObject, public I_Base_Client_Interface
{
	GENERATED_BODY()

public:

	/**
	 * sends the id of a selected mesh-object to the server
	 * @return false if channel not ready or sending failed
	 */
	UFUNCTION(BlueprintCallable, Category = "Selection")
	bool send_selection(const FString& mesh_id);

	virtual void stop_Implementation() override {}
	virtual void state_change_Implementation(connection_state old_state, connection_state new_state) override {}

private:
	std::unique_ptr<generated::selection_com::Stub> stub;

	BASE_CLIENT_BODY(
		[this](const std::shared_ptr<grpc::Channel>& ch)
		{
			stub = generated::selection_com::NewStub(ch);
		}
	)
};