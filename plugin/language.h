/**
 * @brief      Language container for HENkaku Settings
 */
#ifndef LANGUAGE_HEADER
#define LANGUAGE_HEADER

typedef struct {
  char *msg_henkaku_settings;
  char *msg_enable_psn_spoofing;
  char *msg_enable_unsafe_homebrew;
  char *msg_unsafe_homebrew_description;
  char *msg_enable_version_spoofing;
  char *msg_spoofed_version;
  char *msg_button_behaviour;
  char *msg_button_enter;
  char *msg_button_cancel;
} language_container_t;

language_container_t language_english = {
  "HENkaku Settings",
  "Enable PSN spoofing",
  "Enable Unsafe Homebrew",
  "Unsafe homebrews can damage your device permanently, if they are misused, misconfigured, or malicious. Please take caution when installing them.",
  "Enable Version Spoofing",
  "Spoofed Version",
  "○ Button Behavior",
  "Enter",
  "Cancel"
};

#endif // LANGUAGE_HEADER
