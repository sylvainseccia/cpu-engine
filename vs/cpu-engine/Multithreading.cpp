#include "stdafx.h"

void cpu_thread_job::OnCallback()
{
	m_quitRequest = false;

	// cpu_thread
	while ( true )
	{
		// Waiting next job
		WaitForSingleObject(m_hEventStart, INFINITE);
		if ( m_quitRequest )
			break;

		// Job
		while ( true )
		{
			int index = cpu.m_nextTile.fetch_add(1, std::memory_order_relaxed);
			if ( index>=m_count )
				break;

			cpu.Render_Tile(index);

#ifdef CONFIG_MT_DEBUG
			pEngine->Present();
			Sleep(100);
#endif
		}

		// End
		SetEvent(m_hEventEnd);
	}
}
