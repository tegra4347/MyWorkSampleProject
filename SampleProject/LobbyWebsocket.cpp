bool FJCNetworkResource::ConnectionSvrInit()
{
	// 이미 유효한 경우 False
	if (webSocket.IsValid()) {
		return false;
	}
	// 커넥션 서버 URL이 존재하는지
	if (connectionUrl.IsEmpty()) {
		UE_LOG(LogJCServer, Log, TEXT("ConnectionServer connectionServerURL IsEmpty"));
		return false;
	}
	// 서버 토큰 값이 존재 하는지
	if (sessionKey.IsEmpty()) {
		UE_LOG(LogJCServer, Log, TEXT("ConnectionServer sessionKey IsEmpty"));
		return false;
	}
	// 해당 캐릭터의 어카운트 ID가 존재하는지
	if (accountID.IsEmpty()) {
		UE_LOG(LogJCServer, Log, TEXT("ConnectionServer accountID IsEmpty"));
		return false;
	}

	const FString ServerProtocol = TEXT("ws");
	const FString ServerURL = FString::Printf(TEXT("ws://%s/client"), *connectionUrl);
	UE_LOG(LogJCServer, Log, TEXT("ConnectionServer WebSocket Address : %s"), *connectionUrl);

	TMap<FString, FString> header;
	header.Add("ID", accountID);
	header.Add("Auth", sessionKey);

	//웹소켓 생성
	webSocket = FWebSocketsModule::Get().CreateWebSocket(ServerURL, ServerProtocol, header);
	if (!webSocket.IsValid()) {
		return false;
	}

	webSocket->OnConnected().AddRaw(this, &FJCNetworkResource::OnConnected);
	webSocket->OnConnectionError().AddRaw(this, &FJCNetworkResource::OnConnectionError);
	webSocket->OnClosed().AddRaw(this, &FJCNetworkResource::OnClosed);
	webSocket->OnMessage().AddRaw(this, &FJCNetworkResource::OnMessage);

	return true;
}

template <typename T>
int64 FJCNetworkResource::ConnectionSvrSend(const T& message, EServerMessage resType, DELEGATE_Message& delegate)
{
	if (webSocket.IsValid() == false || webSocket->IsConnected() == false)
	{
		UE_LOG(LogJCServer, Error, TEXT("ConnectionServer is not connected."));
		return SERVER_NET_ERROR_CODES::ErrorDisconnect;
	}

	FString ContentBodyString = TEXT("");
	if (!JCNetworkUtil::GetJsonStringFromStruct(message, ContentBodyString))
	{
		UE_LOG(LogJCServer, Error, TEXT("ConnectionServer Request GetJsonStringFromStruct Failed"));
		return SERVER_NET_ERROR_CODES::ErrorJsonParse;
	}

	FWebSocketMessage packet;
	// 메시지 타입
	packet.Type = (int)T::MessageType();
	// 메시지 번호
	packet.Seq = NextSequence;
	// 패킷 데이터
	packet.Body = ContentBodyString;

	FString packetJsonString = TEXT("");
	if (!JCNetworkUtil::GetJsonStringFromStruct<FWebSocketMessage>(packet, packetJsonString))
	{
		UE_LOG(LogJCServer, Error, TEXT("ConnectionServer Request GetJsonStringFromStruct Failed"));
		return SERVER_NET_ERROR_CODES::ErrorJsonParse;
	}

	d_messages.Add(resType, delegate);

	webSocket->Send(packetJsonString);
	NextSequence += 1; // 보낼때마다 증가

	return static_cast<int64>(packet.Seq);
}