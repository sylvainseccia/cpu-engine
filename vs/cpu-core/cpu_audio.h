#pragma once

struct cpu_audio
{
public:
	IXAudio2* pAudio;
	IXAudio2MasteringVoice* pVoice;

public:
	cpu_audio();
	~cpu_audio();

	static cpu_audio& GetInstance();

	bool Initialize();
	void Uninitialize();
};
