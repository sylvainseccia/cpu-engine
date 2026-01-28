#pragma once

// Sylvain Seccia
// https://www.seccia.com

// Include
#include "../cpu-render/cpu-render.h"

// Forward declarations
class cpu_engine;
class cpu_job;

// Singletons
#define cpuEngine						cpu_engine::GetInstance()
#define cpuWindow						(*cpu_engine::GetInstance().GetWindow())
#define cpuDevice						(*cpu_engine::GetInstance().GetDevice())
#define cpuApp							App::GetInstance()

// Macro
#define CPU_JOBS(j)						{m_nextTile=0;for(size_t i=0;i<(j).size();i++)(j)[i].GetThread()->PostStartEvent(&(j)[i]);for(size_t i=0;i<(j).size();i++)(j)[i].GetThread()->WaitEndEvent();}
#define CPU_RUN							cpu::Run<cpu_engine, App>
#define CPU_CALLBACK_START(method)		cpuEngine.GetCallback()->onStart.Set(this, &App::method)
#define CPU_CALLBACK_UPDATE(method)		cpuEngine.GetCallback()->onUpdate.Set(this, &App::method)
#define CPU_CALLBACK_EXIT(method)		cpuEngine.GetCallback()->onExit.Set(this, &App::method)
#define CPU_CALLBACK_RENDER(method)		cpuEngine.GetCallback()->onRender.Set(this, &App::method)
#define CPU_CALLBACK_START_EX(method)	cpuEngine.GetCallback()->onStart.Set(this, &method)
#define CPU_CALLBACK_UPDATE_EX(method)	cpuEngine.GetCallback()->onUpdate.Set(this, &method)
#define CPU_CALLBACK_EXIT_EX(method)	cpuEngine.GetCallback()->onExit.Set(this, &method)
#define CPU_CALLBACK_RENDER_EX(method)	cpuEngine.GetCallback()->onRender.Set(this, &method)

// Clear
#define CPU_CLEAR_NONE					0
#define CPU_CLEAR_COLOR					1
#define CPU_CLEAR_SKY					2

// Pass
#define CPU_PASS_CLEAR_BEGIN			10
#define CPU_PASS_CLEAR_END				11
#define CPU_PASS_ENTITY_BEGIN			20
#define CPU_PASS_ENTITY_END				21
#define CPU_PASS_PARTICLE_BEGIN			30
#define CPU_PASS_PARTICLE_END			31
#define CPU_PASS_UI_BEGIN				40
#define CPU_PASS_UI_END					41
#define CPU_PASS_CURSOR_BEGIN			50
#define CPU_PASS_CURSOR_END				51

// Engine
#include "cpu_global.h"
#include "cpu_manager.h"
#include "cpu_fsm.h"
#include "cpu_entity.h"
#include "cpu_hit.h"
#include "cpu_stats.h"
#include "cpu_callback.h"
#include "cpu_thread_job.h"
#include "cpu_job.h"
#include "cpu_engine.h"
