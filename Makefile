CC	= gcc
L_FLAGS	= `pkg-config --cflags --libs gtk+-2.0`


gui_final: gui_final.c gui_final.h
	$(CC) -Wall -o gui_final gui_final.c $(L_FLAGS)

data_latch: data_latch.c ece586_proj.h
	$(CC) -Wall -o data_latch data_latch.c $(L_FLAGS)

toolbar: toolbar.c
	$(CC) -o toolbar toolbar.c $(L_FLAGS)

menu: menu.c
	$(CC) -o menu menu.c $(L_FLAGS)

counter: counter.c
	$(CC) -o counter counter.c $(L_FLAGS)

window: window.c
	$(CC) -o window window.c $(L_FLAGS)

base: base.c
	$(CC) -o base base.c $(L_FLAGS)

