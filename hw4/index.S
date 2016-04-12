	.global index_payload
	.global index_payload_size
	.section .rodata
index_payload:
	.incbin "index.html"
1:
index_payload_size:
	.int 1b - index_payload
