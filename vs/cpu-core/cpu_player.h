#pragma once

struct cpu_player : public cpu_object
{
protected:
	cpu_sound* pSound;
	bool loop;
	bool pause;
	float volume;
	float pan;
	ui64 previous;
	ui64 current;

public:
	cpu_player();
	~cpu_player();

	void Update();

	void SetSound(cpu_sound* pSnd);
	void SetLoop(bool isLoop = true);
	bool IsLoop();
	void SetVolume(float level = 1.0f);
	float GetVolume();
	void SetPan(float level = 0.0f);
	float GetPan();

	bool Play();
	bool Pause();
	bool Resume();
	void Stop();
	bool IsPlaying();
	bool IsPause();
	float GetPosition();
	ui64 GetSamplePosition();

protected:
	void Reset();
	void UpdateVolume();
};
