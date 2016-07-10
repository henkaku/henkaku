void __attribute__ ((section (".text.start"))) user_payload(int args, unsigned *argp) {
	unsigned SceWebBrowser_base = argp[0];
	unsigned SceLibKernel_base = argp[1];
	// void (*debug_print)() = SceWebBrowser_base + 0xC2A44;
	void (*kill_me)() = SceLibKernel_base + 0x684c;
	// debug_print("hello from the browser!\n");
	kill_me();
	while (1) {}
}
