mov	sp, 0xf000
mov	ss, sp

mov ax, 30
push ax
add cx, 5
mov ax, cx
push cx
mov cx, 99
cmp ax, cx

pop cx
pop ax

mov dx, ax
mov dx, cx