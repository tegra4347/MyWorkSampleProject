
void AJCGameState::SuddenEventCheck()
{
	if (HasAuthority() == false)
		return;

	if (NowSuddenEventID == 0) // 현재 진행중인 돌발이벤트가 없으면
	{
		if (SuddenEventDataList.Num() > 0)
		{
			// 현재 시간을 가져온다
			FDateTime nowtime = FDateTime::UtcNow();

			// 이벤트 진행할 월드 아이디를 가져온다
			int worldid = GetWorldID();

			// 진행 중인 이벤트가 있을 경우
			if (PreSetEventID != 0)
			{
				FDateTime EventStartTime{ FDateTime(0) };

				// 현재 nottime 의 year , month , day 를 넣고 EventData 의 hour minute 
				FString EventStartTimeString = FString::Printf(TEXT("%d.%d.%d-%d.%d.00"), nowtime.GetYear(), nowtime.GetMonth(), nowtime.GetDay(), CurrentEventData.Event_StartTime_hour, CurrentEventData.Event_StartTime_min);

				if (!FDateTime::Parse(EventStartTimeString, EventStartTime))
				{
					UE_LOG(LogClass, Warning, TEXT("Time Parse Error EventStartTimeString %s"), *EventStartTimeString);
				}

				FTimespan remaintime = FTimespan(EventStartTime - nowtime);

				int remainssecond = remaintime.GetTotalSeconds();

				int endsecond = (CurrentEventData.Event_RunTime_min - 1) * 60; //종료 시간(분)을 -1 을 빼고  second 로 변경 

				if (IsSuddenEvent_Preseting)
				{
					if (PreSetEventID == CurrentEventData.ID)
					{
						RemainStartTime = remainssecond;
					}
				}

				if (remainssecond <= 0 && remainssecond >= -endsecond) { // 종료 시간 1분 전까지 라면 이벤트 시작 

					FDateTime EventEndTime = EventStartTime + FTimespan(0, CurrentEventData.Event_RunTime_min, 0);

					// 시작 
					SuddenEventStart(CurrentEventData.ID, EventEndTime);
				}
			}
			else
			{
				// 이벤트 데이터 리스트를 탐색한다.
				for (auto& elem : SuddenEventDataList)
				{
					if (worldid != elem.World_ID)  // 현재 월드 아이디랑 다르면 그냥 continue
					{
						continue;
					}

					FDateTime EventStartTime{ FDateTime(0) };

					// 현재 nowtime 의 year , month , day 를 넣고 EventData 의 hour minute 
					FString EventStartTimeString = FString::Printf(TEXT("%d.%d.%d-%d.%d.00"), nowtime.GetYear(), nowtime.GetMonth(), nowtime.GetDay(), elem.Event_StartTime_hour, elem.Event_StartTime_min);

					if (!FDateTime::Parse(EventStartTimeString, EventStartTime))
					{
						UE_LOG(LogClass, Warning, TEXT("Time Parse Error EventStartTimeString %s"), *EventStartTimeString);
						continue;
					}

					FTimespan remaintime = FTimespan(EventStartTime - nowtime);

					int remainssecond = remaintime.GetTotalSeconds();

					int endsecond = (elem.Event_RunTime_min - 1) * 60; //종료 시간(분)을 -1 을 빼고  second 로 변경 

					if (IsSuddenEvent_Preseting)
					{
						if (PreSetEventID == elem.ID)
						{
							RemainStartTime = remainssecond;
						}
					}

					// 5분전에 스폰 관련된 정보 미리 세팅
					if (IsSuddenEvent_Preseting == false && remainssecond > 0 && remainssecond <= 300)
					{
						UE_LOG(LogTemp, Log, TEXT("Sudden Event 5 minute ago ID:%d , worldID:%d, EventStartTime:%s   "), elem.ID, worldid, *(EventStartTime.ToString()));

						// 보스 몬스터의 정보와 스폰될 포인트를 미리 세팅한다
						PresetSuddenEvent_SpawnInfo(elem);
						// 진행될 이벤트 아이디를 세팅한다
						PreSetEventID = elem.ID;
						CurrentEventData = elem;
					}
				}
			}
		}

	}
	else if (NowSuddenEventID > 0) // 현재 진행중인 돌발이벤트가 있으면 종료시간 체크 
	{

		// 현재 시간 
		FDateTime nowtime = FDateTime::UtcNow();
		// 남은 시간
		FTimespan remaintime = FTimespan(NowSuddenEventEndTime - nowtime);

		RemainEventProgressTime = remaintime.GetTotalSeconds();

		if (SuddenEvent_SpawnPoint)
		{
			if (SuddenEvent_SpawnPoint->GetSuddenEventCharacter_HPPercent() <= 0.05f)
			{
				// 성공 종료
				SuddenEventEnd(true);

				// 클라에 성공 이벤트 호출
				SuddenEvent_End_Client();
				return;
			}
		}

		if (RemainEventProgressTime <= 0)
		{
			// 실패 종료
			SuddenEventEnd(false);

			// 클라에 실패 이벤트 호출
			SuddenEvent_End_Client();
			return;
		}
	}
	// 치트로 발생한 이벤트
	else if (NowSuddenEventID == -1)
	{
		// 현재 시간 
		FDateTime nowtime = FDateTime::UtcNow();

		FTimespan remaintime = FTimespan(NowSuddenEventEndTime - nowtime);

		RemainEventProgressTime = remaintime.GetTotalSeconds();

		if (SuddenEvent_SpawnPoint)
		{
			if (SuddenEvent_SpawnPoint->GetSuddenEventCharacter_HPPercent() <= 0.05f)
			{
				// 성공 종료
				SuddenEventEnd_Cheat(true);

				// 클라에 성공 이벤트 호출
				SuddenEvent_End_Client();
				return;
			}
		}

		if (RemainEventProgressTime <= 0)
		{
			// 실패 종료
			SuddenEventEnd_Cheat(false);

			// 클라에 실패 이벤트 호출
			SuddenEvent_End_Client();
			return;
		}
	}
}
void AJCGameState::PresetSuddenEvent_SpawnInfo(const FSuddenEventData& SuddenEventData)
{
	AJCSpawnPointBase* TempSpawnBase = nullptr;
	// 랜덤 포인트 스폰을 위해 랜덤 인덱스 생성
	int RandomIndex = FMath::RandRange(0, 2);
	for (TActorIterator<AJCSpawnPointTimeEvent> actorIter(GetWorld()); actorIter; ++actorIter)
	{
		TempSpawnBase = *actorIter;

		UJCSpawnConfig_OutBreakNPC* SpawnConfig = Cast<UJCSpawnConfig_OutBreakNPC>(TempSpawnBase->GetSpawnConfig());

		if (SpawnConfig)
		{
			// 해당 레벨 섹터 아이디와 인덱스가 동일 할 경우에
			if (SpawnConfig->GetSectorID() == SuddenEventData.Sector_ID && SpawnConfig->GetRandomIndex() == RandomIndex)
			{
				IsCurrentEvent_RareNPC = false;

				float ResultRareNPCPercent = FMath::RandRange(0.0f, 100.0f);

				if (ResultRareNPCPercent <= SuddenEventData.Race_Event_Percentage)
					IsCurrentEvent_RareNPC = true;

				// 해당 스포너에 이벤트 NPC 정보 세팅
				SpawnConfig->PresetConfigInfo(SuddenEventData.NPC_ID_1, SuddenEventData.NPC_ID_2, IsCurrentEvent_RareNPC, SuddenEventData.NPC_Level);

				IsSuddenEvent_Preseting = true;
				SuddenEvent_SpawnPoint = *actorIter;

				// 진행할 이벤트 정보 세팅
				CurrentEventData_Client.EventID = SuddenEventData.ID;
				CurrentEventData_Client.World_ID = SuddenEventData.World_ID;
				CurrentEventData_Client.Sector_ID = SuddenEventData.Sector_ID;
				CurrentEventData_Client.IsRareNPC = false;
				CurrentEventData_Client.SpawnerLoc = TempSpawnBase->GetActorLocation();

				// RareNPC 여부에 따라 NPCID 세팅
				if (IsCurrentEvent_RareNPC == false)
				{
					CurrentEventData_Client.CurrentEventRewardID = SuddenEventData.Reward_Group_ID_1;
					CurrentEventData_Client.NPCID = SuddenEventData.NPC_ID_1;
				}
				else
				{
					CurrentEventData_Client.CurrentEventRewardID = SuddenEventData.Reward_Group_ID_2;
					CurrentEventData_Client.NPCID = SuddenEventData.NPC_ID_2;
				}

				break;
			}
		}
	}
}

void AJCGameState::PresetSuddenEvent_SpawnInfo_Cheat(const FSuddenEventData& SuddenEventData, int RandomIndex)
{
	AJCSpawnPointBase* TempSpawnBase = nullptr;

	for (TActorIterator<AJCSpawnPointTimeEvent> actorIter(GetWorld()); actorIter; ++actorIter)
	{
		TempSpawnBase = *actorIter;

		UJCSpawnConfig_OutBreakNPC* SpawnConfig = Cast<UJCSpawnConfig_OutBreakNPC>(TempSpawnBase->GetSpawnConfig());

		if (SpawnConfig)
		{
			if (SpawnConfig->GetSectorID() == SuddenEventData.Sector_ID && SpawnConfig->GetRandomIndex() == RandomIndex)
			{
				SpawnConfig->PresetConfigInfo(SuddenEventData.NPC_ID_1, SuddenEventData.NPC_ID_2, false, SuddenEventData.NPC_Level);

				IsSuddenEvent_Preseting = true;
				SuddenEvent_SpawnPoint = *actorIter;

				PreSetEventID = -1;

				CurrentEventData_Client.EventID = -1;
				CurrentEventData_Client.World_ID = SuddenEventData.World_ID;
				CurrentEventData_Client.Sector_ID = SuddenEventData.Sector_ID;
				CurrentEventData_Client.IsRareNPC = false;
				CurrentEventData_Client.CurrentEventRewardID = SuddenEventData.Reward_Group_ID_1;
				CurrentEventData_Client.SpawnerLoc = TempSpawnBase->GetActorLocation();
				CurrentEventData_Client.NPCID = SuddenEventData.NPC_ID_1;
			}
			break;
		}
	}
}

void AJCGameState::SuddenEventEnd(bool IsSuccess)
{
	// 돌발 이벤트 끝
	UE_LOG(LogTemp, Log, TEXT("SuddenEventEnd SuddenEventID: %d"), NowSuddenEventID);
	NowSuddenEventID = 0;

	////스폰 관련 상태값 초기화/////
	IsSuddenEvent_Preseting = false;
	if (SuddenEvent_SpawnPoint)
	{
		SuddenEvent_SpawnPoint->EndOutBreakEvent(IsSuccess);	// 돌발이벤트 Spawn Confing 초기화
		SuddenEvent_SpawnPoint = nullptr;
	}

	auto GameInst = Util::GetGameInstance(GetWorld());

	if (GameInst)
	{
		TArray<FWeb_SuddenEventDamage> DmgList;

		float TotalDamage = 0;
		// 필드보스에게 입힌 총 피해를 구한다.
		for (int i = 0; i < PlayerArray.Num(); ++i)
		{
			auto PlayerState = Cast<AJCPlayerState>(PlayerArray[i]);

			if (PlayerState->GetTimeEventDamage() >= 1.f)
			{
				TotalDamage += PlayerState->GetTimeEventDamage();
			}
		}
		// 플레이어별 데미지 퍼센트 , 캐릭터 아이디 , 어카운트 아이디를 가져온다.
		for (int i = 0; i < PlayerArray.Num(); ++i)
		{
			auto PlayerState = Cast<AJCPlayerState>(PlayerArray[i]);

			if (PlayerState)
			{
				if (PlayerState->GetTimeEventDamage() >= 1.f)
				{
					FWeb_SuddenEventDamage Temp;

					auto PlayerCtrl = Cast<AJCPlayerController>(PlayerState->GetOwner());
					if (PlayerCtrl)
					{
						Temp.CharacterID = PlayerCtrl->GetCharacterId().Get();
						Temp.AccountID = PlayerCtrl->GetAccountId().Get();
						Temp.DamagePercent = (PlayerState->GetTimeEventDamage() / TotalDamage) * 100.f;

						DmgList.Add(Temp);
					}
				}
			}
		}

		// 데미지 리스트를 소팅한다(데미지 순위에 따라 보상 차등 지급)
		DmgList.Sort([](const FWeb_SuddenEventDamage& A, const FWeb_SuddenEventDamage& B)
			{
				return A.DamagePercent > B.DamagePercent;
			});

		// 이벤트 정보 , 소팅된 데미지 리스트를 웹서버로 결과 패킷 보낸다.
		if (IsCurrentEvent_RareNPC)
			SuddenEventEndResult(CurrentEventData.ID, CurrentEventData.World_ID, CurrentEventData.Reward_Group_ID_2, IsSuccess, GameInst->ServerGroupID, DmgList);
		else
			SuddenEventEndResult(CurrentEventData.ID, CurrentEventData.World_ID, CurrentEventData.Reward_Group_ID_1, IsSuccess, GameInst->ServerGroupID, DmgList);
	}

	PreSetEventID = 0;

	for (int i = 0; i < PlayerArray.Num(); ++i)
	{
		auto PlayerState = Cast<AJCPlayerState>(PlayerArray[i]);

		if (PlayerState)
			PlayerState->ClearTimeEventDamage();
	}

	CurrentEventData_Client.EventID = 0;

	RemainStartTime = 0;
	RemainEventProgressTime = 0;
	////////////////////////////////

	auto WorldState = Util::GetWorldState(GetWorld());

	if (WorldState)
		WorldState->HideBossHPGuage();
}