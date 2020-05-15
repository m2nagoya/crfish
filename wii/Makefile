# コンパイルなどが記載されていませんが，
# makeのデフォルト動作を使っています．
#
CC = gcc
TEX = platex

CFLAGS = -Wall $(COPTIONS)

LDFLAGS = 
LOADLIBES = 
LDLIBS = -lm -lSDL -lSDL_image -lSDL_ttf -lSDL_gfx -lSDL_mixer -lopencv_core -lopencv_imgproc -lopencv_highgui -lcwiimote -D_ENABLE_TILT -D_ENABLE_FORCE -L/usr/lib

TARGET = game_main
OBJS = $(TARGET).o game_system.o game_window.o 
HEADS = game_system.h

# 課題に取り組むにはこのターゲットで
#   -g3 はデバッグ情報を埋め込むオプション
#
debug: CPPFLAGS = 
debug: COPTIONS = -g3
debug: $(TARGET)

# デバッグ情報を埋め込まないターゲット
#   -O3 は最適化オプション
#
release: CPPFLAGS = -DNDEBUG
release: COPTIONS = -O3 
release: $(TARGET)

#
$(TARGET): $(OBJS)
$(OBJS): $(HEADS)

# レポート作成用ターゲット
# 例えば repo.tex があるとき，
#  make repo.pdf でpdfまで生成する
#
%.pdf:%.dvi
	dvipdfmx $<

# コンパイルをやり直したい時などに，
# 中間ファイルなどを削除するターゲット
#   使用例： make clean
#
.PHONY: clean
clean:
	@rm -f $(TARGET) $(OBJS) core *~
