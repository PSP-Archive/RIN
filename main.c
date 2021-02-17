#include "main.h"

volatile int bSleep=0;
int bMenu=0;

char RinPath[MAX_PATH];
char RomPath[MAX_PATH];
char RomName[MAX_NAME];
char SavePath[MAX_PATH];
char CheatPath[MAX_PATH];

void set_cpu_clock(int n)
{
	if(n==0)
		scePowerSetClockFrequency(222,222,111);
	else if(n==1)
		scePowerSetClockFrequency(266,266,133);
	else if(n==2)
		scePowerSetClockFrequency(333,333,166);
}

// -----------------------------------------------------------------------------

// �z�[���{�^���I�����ɃR�[���o�b�N
int exit_callback(void)
{
	bSleep=1;
	set_cpu_clock(0);
	save_config();
	if (rom_get_loaded() && rom_has_battery())
		save_sram(get_sram(), rom_get_info()->ram_size);
	
	sceKernelExitGame();
	return 0;
}

// �d���X�C�b�`���쎞��s����ɃR�[���o�b�N�B
// ���̊֐����܂����s���ł��T�X�y���h�E�X�^���o�C�ɓ���\��������B
void power_callback(int unknown, int pwrflags)
{
	//if(pwrflags & (POWER_CB_SUSPEND|POWER_CB_STANDBY)){
	if(pwrflags & POWER_CB_POWER){
		if (!bSleep){
			bSleep=1;

			// �t�@�C���A�N�Z�X���ɃT�X�y���h�E�X�^���o�C�����
			// 0byte�̃Z�[�u�t�@�C�����ł��Ă��܂����Ƃ�����̂ŁA
			// �������ݒ��̓T�X�y���h�E�X�^���o�C�𖳌����B
			sceKernelPowerLock(0);
			set_cpu_clock(0);
			save_config();
			if (rom_get_loaded() && rom_has_battery())
				save_sram(get_sram(), rom_get_info()->ram_size);
			sceKernelPowerUnlock(0);
		}
	}
	if(pwrflags & POWER_CB_BATLOW){
		//renderer_set_msg("PSP Battery is Low!");
		if (!bSleep){
			bSleep=1;

			sceKernelPowerLock(0);
			set_cpu_clock(0);
			save_config();
			if (rom_get_loaded() && rom_has_battery())
				save_sram(get_sram(), rom_get_info()->ram_size);
			sceKernelPowerUnlock(0);
			
			// �����T�X�y���h�B
			// �o�b�e������10%��؂�p���[�����v���_�ł��n�߂�ƁA
			// ���삪�ɒ[�ɒx���Ȃ�t���[�Y������Z�[�u�ł��Ȃ��Ȃ����肷��B
			// �s�̃Q�[���ł�0%�܂Ŏg���Ă�悤�Ȃ̂���B
			scePowerRequestSuspend(); 
		}
	}
	if(pwrflags & POWER_CB_RESCOMP){
		bSleep=0;
	}

	// �R�[���o�b�N�֐��̍ēo�^
	// �i��x�Ă΂ꂽ��ēo�^���Ă����Ȃ��Ǝ��ɃR�[���o�b�N����Ȃ��j
	int cbid = sceKernelCreateCallback("Power Callback", power_callback, NULL);
	scePowerRegisterCallback(0, cbid);
}

// �|�[�����O�p�X���b�h
int CallbackThread(int args, void *argp)
{
	int cbid;
	
	// �R�[���o�b�N�֐��̓o�^
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	cbid = sceKernelCreateCallback("Power Callback", power_callback, NULL);
	scePowerRegisterCallback(0, cbid);
	
	// �|�[�����O
	sceKernelSleepThreadCB();

	return 0;
}

int SetupCallbacks(void)
{
	int thid = 0;
	
	// �|�[�����O�p�X���b�h�̐���
	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
		sceKernelStartThread(thid, 0, 0);
	
	return thid;
}

// -----------------------------------------------------------------------------

void mainloop(void)
{
#ifdef DEBUG
	unsigned long framecount=0;
	unsigned long lastclock=sceKernelLibcClock();
	unsigned long lasttick=lastclock;
#endif
	const unsigned int sync_time=16666;
	unsigned long cur_time = sceKernelLibcClock();
	unsigned long cur_time_bak = cur_time;
	unsigned long prev_time = cur_time;
	unsigned long next_time = cur_time + sync_time;
	unsigned long waitfc=0;
	int line, turbo_bak=0;

	for(;;) {
		for(line=0; line<154; line++)
			gb_run();
		
		cur_time = sceKernelLibcClock();
#ifdef DEBUG
		framecount++;
		if (framecount>=60) {
			unsigned long l;

			//�t���[�����[�g�̎w�W�B60�t���[���ł�����������(usec)���P�U�i�ŕ\���B
			//�t���t���[����0x000f4240�ƂȂ�A�傫���ƒx�����ƂɂȂ�B�𑜓x�������̂͊��فB - LCK
			framecount=0;
			
			pgcLocate(50,0);
			pgcPuthex8(cur_time-lasttick);
			lasttick=cur_time;
			
			pgcLocate(50,2);
			pgcPuthex8(c_regs_PC);
			l=(cpu_read(c_regs_PC)<<24)+(cpu_read(c_regs_PC+1)<<16)+(cpu_read(c_regs_PC+2)<<8)+(cpu_read(c_regs_PC+3));
			pgcLocate(50,3);
			pgcPuthex8(l);

			pgcLocate(50,5);
			pgcPuthex8(cur_time-lastclock);
			lastclock=cur_time;

			pgcLocate(50,25);
			pgcPuthex8(g_regs.IF);
			pgcLocate(50,26);
			pgcPuthex8(g_regs.IE);
			extern byte c_regs_I;
			pgcLocate(50,27);
			pgcPuthex8(c_regs_I);
			
			// kmg
			pgcLocate(2, 2);
			pgcPuthex8(paddata.analog[CTRL_ANALOG_X]);
			pgcLocate(2, 3);
			pgcPuthex8(paddata.analog[CTRL_ANALOG_Y]);
		}
		pgScreenFlip();
#else
		if (bTurbo){
			turbo_bak = 1;
			skip++;
			if (skip > 9){
				skip = 0;
				prev_time = cur_time;
			}
		}else if (cur_time < cur_time_bak){
			prev_time = cur_time;
			skip=0;
		}else if (cur_time > next_time){
			skip++;
			if(skip > setting.frameskip){
				skip=0;
				if(setting.vsync){
					sceDisplayWaitVblank();
					cur_time = sceKernelLibcClock();
				}
				prev_time = cur_time;
			}
		}else{
			if(setting.vsync){
				sceDisplayWaitVblank();
				cur_time = sceKernelLibcClock();
				prev_time = cur_time;
			}else{
				waitfc++;
				while(cur_time < prev_time+10000*(skip+1))
					cur_time = sceKernelLibcClock();
				if (!(waitfc&3)){
					while(cur_time < next_time)
						cur_time = sceKernelLibcClock();
				}
				prev_time = next_time;
			}
			skip=0;
		}
		cur_time_bak = cur_time;
		next_time = prev_time + sync_time * (skip+1);
		if (setting.vsync && !bTurbo){
			if (turbo_bak)
				turbo_bak = 0;
			else if (skip==0) 
				pgScreenFlip();
		}else{
			if (now_frame==0) pgScreenFlip();
		}
#endif
		
		// ���j���[
		if(bMenu){
			wavout_enable=0;
			set_cpu_clock(0);
			rin_menu();
			set_cpu_clock(setting.cpu_clock);
			if(setting.sound) wavout_enable=1;

			cur_time = sceKernelLibcClock();
			prev_time = cur_time;
			next_time = cur_time + sync_time;
			skip=0;
			bMenu = 0;
		}
		
		// �X���[�v
		if(bSleep){
			wavout_enable=0;
			while(bSleep)
				pgWaitV();
			set_cpu_clock(setting.cpu_clock);
			if(setting.sound) wavout_enable=1;

			cur_time = sceKernelLibcClock();
			prev_time = cur_time;
			next_time = cur_time + sync_time;
			skip=0;
		}
	}
}

int xmain(int argc, char *argv)
{
	int romsize, ramsize;
	char *p, tmp[MAX_PATH];
	
	pgInit();

	strcpy(RinPath, argv);
	p = strrchr(RinPath, '/');
	*++p = 0;
	sprintf(CheatPath, "%sCHEAT/", RinPath);

	SetupCallbacks();
	pgScreenFrame(2,0);
	wavoutInit();
	
	load_config();
	bBitmap = load_menu_bg();
	if(bBitmap) bgbright_change();
	if(setting.sound) wavout_enable=1;

	strcpy(tmp,RinPath);
	strcat(tmp,"SAVE");
	sceIoMkdir(tmp,0777);
	strcpy(tmp,RinPath);
	strcat(tmp,"CHEAT");
	sceIoMkdir(tmp,0777);

	gb_init();
	strcpy(RomPath,setting.lastpath);
	for(;;){
		if (!getFilePath(RomPath,EXT_GB|EXT_GZ|EXT_ZIP))
			continue;
		strcpy(tmp, RomPath);
		*(strrchr(tmp,'/')+1) = 0;
		strcpy(setting.lastpath, tmp);

		// �w�肵���t�@�C�������[�h����B by ruka
		romsize = load_rom(RomPath);
		if (!romsize){
			strcpy(filer_msg,"ROM Load Failed");
			continue;
		}
		ramsize = load_sram(sram_space, sizeof(sram_space));
		if (!gb_load_rom(rom_image, romsize, sram_space, ramsize)){
			strcpy(filer_msg,"ROM Load Failed");
			continue;
		}
		
		if(org_gbtype==1)
			renderer_set_msg("ROM TYPE:GB");
		else if(org_gbtype==2)
			renderer_set_msg("ROM TYPE:SGB");
		else if(org_gbtype==3)
			renderer_set_msg("ROM TYPE:GBC");

		break;
	}

	pgFillvram(0);
	pgScreenFlipV();
	pgFillvram(0);
	pgScreenFlipV();

	set_cpu_clock(setting.cpu_clock);

	mainloop();

	return 0;
}

