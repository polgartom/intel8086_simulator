cpu 8086

org 0x0100

%define SCREEN_WIDTH 128

start:
mov bx, title_screen
mov cx, cs:[frame_buffer_bytes]
mov di, 0
copy_title_screen_loop:
    ;mov ax, cs:[bx + di]
    ;mov [di], ax
    mov ah, cs:[bx + di]
    mov [di], ah
    mov al, cs:[bx + di + 1]
    mov [di+1], al
    add di, 2
    loop copy_title_screen_loop
jmp start

screen_size dw SCREEN_WIDTH
frame_buffer_index db 0
frame_buffer_bytes dw 64 ; this is 128*128 but in big-endian 

title_screen:
%include "jura.dat"