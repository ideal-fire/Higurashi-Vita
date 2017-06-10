PSVITAIP=$(VITAIP)

TITLE_ID = VSDK00001
TITLE_ID = HIGURASHI
TARGET   = Higurahsi
OBJS     = ./src/main.o

LIBS = -lSDL2_mixer -lvorbisfile -lvorbis -logg -lSDL2 -lvita2d -lSceDisplay_stub -lSceGxm_stub \
	-lSceSysmodule_stub -lSceCtrl_stub -lScePgf_stub \
	-lSceCommonDialog_stub -lSceAudio_stub -lSceTouch_stub -lfreetype -lpng -ljpeg -lz -lm -lc -llua -lm

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX = $(PREFIX)-g++
CFLAGS  = -Wl,-q -O3 -g -Waddress \
-Warray-bounds=1 \
-Wbool-compare \
-Wchar-subscripts  \
-Wcomment  \
-Wenum-compare \
-Wformat   \
-Wimplicit \
-Wimplicit-int \
-Wimplicit-function-declaration \
-Winit-self \
-Wlogical-not-parentheses \
-Wmain \
-Wmaybe-uninitialized \
-Wmemset-transposed-args \
-Wmisleading-indentation \
-Wmissing-braces \
-Wnarrowing \
-Wnonnull  \
-Wnonnull-compare  \
-Wopenmp-simd \
-Wparentheses  \
-Wreturn-type  \
-Wsequence-point  \
-Wsign-compare \
-Wsizeof-pointer-memaccess \
-Wstrict-aliasing  \
-Wstrict-overflow=1  \
-Wswitch  \
-Wtautological-compare  \
-Wtrigraphs  \
-Wuninitialized  \
-Wunknown-pragmas  \
-Wunused-function  \
-Wunused-label     \
-Wunused-value     \
-Wunused-variable  \
-Wvolatile-register-var
CXXFLAGS = $(CFLAGS)
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

%.vpk: eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE_ID) "$(TARGET)" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin \
	--add VpkContents/sce_sys/icon0.png=sce_sys/icon0.png \
	--add VpkContents/sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png \
	--add VpkContents/sce_sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png \
	--add VpkContents/sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml \
	Higurashi.vpk

eboot.bin: $(TARGET).velf
	vita-make-fself -s $< $@

%.velf: %.elf
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

%.o: %.png
	$(PREFIX)-ld -r -b binary -o $@ $^

clean:
	@rm -rf $(TARGET).vpk $(TARGET).velf $(TARGET).elf $(OBJS) \
		eboot.bin param.sfo

vpksend: $(TARGET).vpk
	curl -T $(TARGET).vpk ftp://$(PSVITAIP):1337/ux0:/_stuffz/
	@echo "Sent."

send: eboot.bin
	curl -T eboot.bin ftp://192.168.1.229:1337/ux0:/app/$(TITLE_ID)/
	@echo "Sent."
