FATE_AAC += fate-aac-al04_44
fate-aac-al04_44: CMD = pcm -i $(SAMPLES)/aac/al04_44.mp4
fate-aac-al04_44: REF = $(SAMPLES)/aac/al04_44.s16

FATE_AAC += fate-aac-al05_44
fate-aac-al05_44: CMD = pcm -i $(SAMPLES)/aac/al05_44.mp4
fate-aac-al05_44: REF = $(SAMPLES)/aac/al05_44.s16

FATE_AAC += fate-aac-al06_44
fate-aac-al06_44: CMD = pcm -i $(SAMPLES)/aac/al06_44.mp4
fate-aac-al06_44: REF = $(SAMPLES)/aac/al06_44.s16

FATE_AAC += fate-aac-al07_96
fate-aac-al07_96: CMD = pcm -i $(SAMPLES)/aac/al07_96.mp4
fate-aac-al07_96: REF = $(SAMPLES)/aac/al07_96.s16

FATE_AAC += fate-aac-al15_44
fate-aac-al15_44: CMD = pcm -i $(SAMPLES)/aac/al15_44.mp4
fate-aac-al15_44: REF = $(SAMPLES)/aac/al15_44.s16

FATE_AAC += fate-aac-al17_44
fate-aac-al17_44: CMD = pcm -i $(SAMPLES)/aac/al17_44.mp4
fate-aac-al17_44: REF = $(SAMPLES)/aac/al17_44.s16

FATE_AAC += fate-aac-al18_44
fate-aac-al18_44: CMD = pcm -i $(SAMPLES)/aac/al18_44.mp4
fate-aac-al18_44: REF = $(SAMPLES)/aac/al18_44.s16

FATE_AAC += fate-aac-am00_88
fate-aac-am00_88: CMD = pcm -i $(SAMPLES)/aac/am00_88.mp4
fate-aac-am00_88: REF = $(SAMPLES)/aac/am00_88.s16

FATE_AAC += fate-aac-am05_44
fate-aac-am05_44: CMD = pcm -i $(SAMPLES)/aac/am05_44.mp4
fate-aac-am05_44: REF = $(SAMPLES)/aac/am05_44.s16

FATE_AAC += fate-aac-al_sbr_hq_cm_48_2
fate-aac-al_sbr_hq_cm_48_2: CMD = pcm -i $(SAMPLES)/aac/al_sbr_cm_48_2.mp4
fate-aac-al_sbr_hq_cm_48_2: REF = $(SAMPLES)/aac/al_sbr_hq_cm_48_2.s16

FATE_AAC += fate-aac-al_sbr_hq_cm_48_5.1
fate-aac-al_sbr_hq_cm_48_5.1: CMD = pcm -i $(SAMPLES)/aac/al_sbr_cm_48_5.1.mp4
fate-aac-al_sbr_hq_cm_48_5.1: REF = $(SAMPLES)/aac/al_sbr_hq_cm_48_5.1.s16

FATE_AAC += fate-aac-al_sbr_ps_06_ur
fate-aac-al_sbr_ps_06_ur: CMD = pcm -i $(SAMPLES)/aac/al_sbr_ps_06_new.mp4
fate-aac-al_sbr_ps_06_ur: REF = $(SAMPLES)/aac/al_sbr_ps_06_ur.s16

FATE_AAC += fate-aac-latm_000000001180bc60
fate-aac-latm_000000001180bc60: CMD = pcm -i $(SAMPLES)/aac/latm_000000001180bc60.mpg
fate-aac-latm_000000001180bc60: REF = $(SAMPLES)/aac/latm_000000001180bc60.s16

FATE_AAC += fate-aac-ap05_48
fate-aac-ap05_48: CMD = pcm -i $(SAMPLES)/aac/ap05_48.mp4
fate-aac-ap05_48: REF = $(SAMPLES)/aac/ap05_48.s16

FATE_AAC += fate-aac-latm_stereo_to_51
fate-aac-latm_stereo_to_51: CMD = pcm -i $(SAMPLES)/aac/latm_stereo_to_51.ts -ac 6
fate-aac-latm_stereo_to_51: REF = $(SAMPLES)/aac/latm_stereo_to_51.s16

fate-aac-ct%: CMD = pcm -i $(SAMPLES)/aac/CT_DecoderCheck/$(@:fate-aac-ct-%=%)
fate-aac-ct%: REF = $(SAMPLES)/aac/CT_DecoderCheck/aacPlusv2.wav

FATE_AAC_CT = sbr_bc-ps_i.3gp  \
              sbr_bic-ps_i.3gp \
              sbr_i-ps_i.aac   \
              sbr_bc-ps_bc.mp4 \
              sbr_bc-ps_i.mp4  \
              sbr_i-ps_bic.mp4 \
              sbr_i-ps_i.mp4

FATE_AAC += $(FATE_AAC_CT:%=fate-aac-ct-%)

FATE_TESTS += $(FATE_AAC)
fate-aac: $(FATE_AAC)
$(FATE_AAC): CMP = oneoff
$(FATE_AAC): FUZZ = 2
