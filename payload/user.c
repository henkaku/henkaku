void __attribute__ ((section (".text.start"))) user_payload(int args, void *argp) {
	unsigned base = ((unsigned*)argp)[0];
	void (*debug_print)() = base + 0xC2A44;
	debug_print("hello from the browser!\n");
	while (1) {}
}
