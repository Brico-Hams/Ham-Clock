#pragma once

#include "NetworkProtocol.h"

constexpr const char* kNetworkPassword = "BricohamsCW";

bool networkBegin();
void networkService();
bool networkPublish(const char* text, const char* callsign);
bool networkReceive(brico::Message& message);
bool networkDecodeEnabled();
bool networkSetDecode(bool enabled);
const char* networkSsid();
bool networkReady();
unsigned networkPending();
