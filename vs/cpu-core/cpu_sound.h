#pragma once

struct cpu_sound
{
public:
	bool loaded;
	WAVEFORMATEX format;
	IXAudio2SourceVoice* pVoice;
	XAUDIO2_BUFFER buffer;
	byte* data;
	int size;

public:
	cpu_sound();
	~cpu_sound();

	bool LoadWave(byte* sourceBuffer, int sourceSize);
	bool LoadRaw(byte* sourceBuffer, int sourceSize);
	bool LoadMp3(byte* sourceBuffer, int sourceSize);
	void Close();
};
