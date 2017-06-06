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
  char *msg_button_behavior;
  char *msg_button_enter;
  char *msg_button_cancel;
  char *msg_reload_taihen_config;
  char *msg_reload_taihen_config_success;
  char *msg_reboot_device;
  char *msg_downloader;
  char *msg_unlink_memory_card;
  char *msg_unlink_memory_card_success;
  char *msg_unlink_memory_card_error;
} language_container_t;

// by Nzaki0716 & SnyFbSx & kirisame0017
language_container_t language_japanese = {  
  "HENkakuの設定",
  "PSNの偽装を有効化",
  "リスクのあるユーザープログラムを有効化",
  "リスクのあるユーザープログラムは誤った操作や設定、または悪意のあるコードにより端末に修復不可能なダメージを与えることがあります。これらをインストールする際は注意してください。",
  "バージョンの偽装を有効化",
  "偽装バージョンの確認",
  "○ボタンの動作設定",
  "決定",
  "キャンセル",
  "taiHEN config.txtのリロード。",
  "taiHEN config.txtのリロードが完了しました。",
  "端末の再起動",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by molecule
language_container_t language_english_us = {
  "HENkaku Settings",
  "Enable PSN spoofing",
  "Enable Unsafe Homebrew",
  "Unsafe homebrews can damage your device permanently, if they are misused, misconfigured, or malicious. Please take caution when installing them.",
  "Enable Version Spoofing",
  "Spoofed Version",
  "○ Button Behavior",
  "Enter",
  "Cancel",
  "Reload taiHEN config.txt",
  "taiHEN config.txt has been successfully reloaded.",
  "Reboot device",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by jokira & devnoname120 & CelesteBlue-dev & chronoss09
language_container_t language_french = {
  "Paramètres de HENkaku",
  "Activer le spoof du PSN",
  "Autoriser l'installation de homebrews non sécurisés",
  "Les homebrews non sécurisés peuvent endommager votre système de façon permanente s'ils sont mal utilisés, mal configurés, ou malveillants. Prenez garde en activant ce paramètre.",
  "Activer le spoof de version",
  "Version spoofée",
  "Action du bouton ○",
  "Valider",
  "Annuler",
  "Recharger le fichier config.txt de taiHEN",
  "Le fichier config.txt de taiHEN a bien été rechargé.",
  "Redémarrer la console",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by EricWeichhart & iamn0tdev
language_container_t language_spanish = {
  "Ajustes de HENkaku",
  "Activar Spoofing para PSN",
  "Activar Homebrew inseguro",
  "Homebrews inseguros pueden dañar tu dispositivo permanentemente si son maliciosos, erróneamente usados o configurados incorrectamente. Por favor, tenga precaución al instalarlos.",
  "Activar Spoofing de versión",
  "Versión para Spoofing",
  "Comportamiento del botón ○",
  "Aceptar",
  "Cancelar",
  "Recargar taiHEN config.txt",
  "taiHEN config.txt ha sido recargado correctamente.",
  "Reiniciar dispositivo",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by Ziusun
language_container_t language_german = {
  "HENkaku-Einstellungen",
  "PSN-Zugang aktivieren",
  "Unsichere Homebrew zulassen",
  "Unsichere Homebrew kann das Gerät dauerhaft beschädigen, sollte sie unsachgemäß eingesetzt und konfiguriert werden oder schadhaft sein. Die Installation bitte immer mit größter Sorgfalt vornehmen.",
  "Andere Firmware vortäuschen",
  "Aktuelle Firmware",
  "Funktion der ○-Taste",
  "Bestätigen",
  "Abbrechen",
  "Die config.txt von taiHEN aktualisieren",
  "Die config.txt von taiHEN wurde erfolgreich aktualisiert.",
  "Gerät neustarten",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by blackjack4it & Checcolino
language_container_t language_italian = {
  "Impostazioni Henkaku",
  "Abilita lo Spoof PSN",
  "Abilita Homebrews non sicuri",
  "Gli Homebrews non sicuri possono danneggiare irrimediabilmente la tua console, se usati/configurati non correttamente o sospetti. Fai molta attenzione prima di procedere alla loro installazione.",
  "Abilita lo Spoof della versione corrente",
  "Versione corrente (Spoof)",
  "Comportamento del tasto ○",
  "OK",
  "Annulla",
  "Ricarica taiHEN config.txt",
  "TaiHEN config.txt ricaricato correttamente.",
  "Riavvia dispositivo",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by ConsoleHax & 2dook
language_container_t language_dutch = {
  "HENkaku Instellingen",
  "Activeer PSN spoofing",
  "Onveilige Homebrew toestaan",
  "Onveilige homebrew kan uw toestel permanent beschadigen indien deze onjuist geconfigureerd of kwaadaardig zijn. Let op met wat u installeert.",
  "Schakel Versie Spoofing in",
  "Spoofed Versie",
  "Gedrag van de ○-knop",
  "Enter",
  "Annuleren",
  "Herlaad taiHEN config.txt",
  "taiHEN config.txt is succesvol herladen.",
  "Herstart apparaat",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by gnmmarechal & menelkir
language_container_t language_portuguese_pt = {
  "Definições do HENkaku",
  "Ativar spoof da PSN",
  "Permitir aplicações inseguras",
  "Aplicações inseguras podem danificar o sistema permanentemente, se forem mal usadas, configuradas ou maliciosas. Tenha cuidado ao instalá-las.",
  "Ativar spoof da versão",
  "Versão do spoof",
  "Ação do botão ○",
  "Entrar",
  "Cancelar",
  "Recarrege o ficheiro config.txt do taiHEN",
  "O ficheiro taiHEN config.txt foi recarregado com sucesso.",
  "Reiniciar o dispositivo",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by Tenek & MuskratDesman
language_container_t language_russian = {
  "Настройки HENkaku",
  "Включить спуфинг PSN",
  "Включить небезопасные приложения",
  "Небезопасные приложения могут повредить ваше устройство навсегда, если они используются неправильно, неправильно настроены или вредоносны. Пожалуйста, проявляйте осторожность при их установке.",
  "Включить подмену версии",
  "Поддельная версия",
  "Поведение кнопки ○",
  "Ввод",
  "Отмена",
  "Перезагрузить taiHEN config.txt",
  "taiHEN config.txt был успешно перезагружен.",
  "Перезапустить устройство",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by TriggerHavoc
language_container_t language_korean = {
  "HENkaku 설정",
  "PSN 스푸핑 활성화",
  "안전하지 않은 홈브류 활성화",
  "만약 홈브류가 오설정, 또는 악성코드등 오용된다면, 안전하지 않은 홈브류 활성화 옵션은 당신의 디바이스에 영구적으로 피해를 줄 수 있습니다. 홈브류를 설치할 때 주의해주세요.",
  "버전 스푸핑 활성화",
  "스푸핑 버전",
  "○ 버튼 설정",
  "확인",
  "취소",
  "taiHEN config.txt를 다시 로드하기",
  "taiHEN config.txt를 성공적으로 로드했습니다.",
  "기기 재부팅",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by iTZQing & FlexingTiger
language_container_t language_chinese_t = {
  "HENkaku設置",
  "啟用PSN偽裝",
  "啟用不安全自制軟件",
  "如果不安全自制軟件被誤用、配置出現錯誤或軟件本身是惡意程序，可能會永久性損壞你的設備。請謹慎安裝！",
  "啟用版本偽裝",
  "偽裝版本",
  "○ 按鍵配置",
  "確定",
  "取消",
  "重新載入 taiHEN config.txt",
  "重新載入 taiHEN config.txt 成功",
  "重啟設備",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by iTZQing & FlexingTiger
language_container_t language_chinese_s = {
  "HENkaku设置",
  "启用PSN伪装",
  "启用不安全自制软件",
  "如果不安全自制软件被误用、配置出现错误或软件本身是恶意程序，可能会永久性损坏你的设备。请谨慎安装！",
  "启用版本伪装",
  "伪装版本",
  "○ 按键配置",
  "确定",
  "取消",
  "重新载入 taiHEN config.txt",
  "重新载入 taiHEN config.txt 成功",
  "重启设备",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by v5000a
language_container_t language_finnish = {
  "HENkakun Asetukset",
  "Ota PSN spooffaaminen käyttöön",
  "Salli vaaralliset homebrew-ohjelmat",
  "Vaaralliset homebrew-ohjelmat voivat vahingoittaa laitettasi pysyvästi, mikäli niitä käytetään väärin, ne konfiguroidaan väärin, tai ne ovat haittaohjelmia. Ole varovainen asentaessasi niitä.",
  "Ota version spooffaaminen käyttöön",
  "Spooffattu versio",
  "○-näppäimen toiminto",
  "Valitse",
  "Peruuta",
  "Lataa taiHEN config.txt uudelleen",
  "taiHEN config.txt uudelleenladattiin onnistuneesti.",
  "Käynnistä laite uudelleen",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by MrOrbital & GizmoTheGreen
language_container_t language_swedish = {
  "HENkaku Inställningar",
  "Aktivera fejkad PSN-inloggning",
  "Tillåt osäker Homebrew",
  "Osäkra Homebrews kan skada din enhet permanent ifall de används på fel sätt, är felkonfigurerade, eller innehåller skadlig kod. Var försiktig vid installation.",
  "Aktivera fejkad version",
  "Fejkad version",
  "Funktion på ○-knapp",
  "Välj",
  "Avbryt",
  "Ladda om taiHEN config.txt",
  "taiHEN config.txt har laddats om.",
  "Starta om enhet",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by coestergaard & andrsnDK
language_container_t language_danish = {
  "HENkaku indstillinger",
  "Aktiver PSN spoofing",
  "Tillad usikre homebrews",
  "Usikre homebrews kan skade din enhed permanent, hvis de bruges forkert, er fejlkonfigurerede eller indeholder skadelig kode. Vær forsigtig når du installerer disse.",
  "Aktiver version spoofing",
  "Spoofed version",
  "○ knap funktion",
  "Vælg",
  "Afbryd",
  "Genindlæs taiHEN config.txt",
  "taiHEN config.txt er blevet geninlæst med succes.",
  "Genstart enhed",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by baniel105 & irchagaming
language_container_t language_norwegian = {
  "HENkaku Instillinger",
  "Aktiver forfalsket PSN-innlogging",
  "Tillat usikker Homebrew",
  "Usikre Homebrews kan ødelegge enhenten din permanent, hvis de er misbrukt, feilkonfigerert, eller inneholder skadelig programvare. Vær forsiktig ved installasjon.",
  "Aktiver forfalsket versjon",
  "Forfalsket versjon",
  "Funksjon på ○-knapp",
  "Velg",
  "Avbryt",
  "Last på nytt taiHEN config.txt",
  "taiHEN config.txt har blitt lastet.",
  "Start enheten på nytt",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by Grzybojad & szczuru & ItsYogSothoth
language_container_t language_polish = {
  "Ustawienia HENkaku",
  "Włącz fałszowanie PSN/SEN (spoofing)",
  "Zezwól na podejrzane Homebrew",
  "Podejrzane aplikacje mogą zawierać złośliwe oprogramowanie, przez co mogą trwale uszkodzić twoje urządzenie, jeżeli zostaną niepoprawnie użyte, bądź źle skonfigurowane. Proszę zachować ostrożność przy ich instalacji.",
  "Włącz fałszowanie wersji oprogramowania (spoofing)",
  "Fałszowana wersja",
  "Akcja przycisku ○",
  "OK",
  "Anuluj",
  "Załaduj ponownie config.txt taiHEN",
  "config.txt taiHEN został pomyślnie załadowany ponownie.",
  "Uruchom ponownie konsolę",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by GrayJack & haouingvalt & menelkir
language_container_t language_portuguese_br = {
  "Configurações do HENkaku",
  "Habilitar mimetização da PSN ",
  "Habilitar softwares 'homebrew' inseguros",
  "Softwares 'homebrew' inseguros podem danificar o seu aparelho permanentemente caso sejam usados ou configurados incorretamente, ou até mesmo maliciosos. Por favor, tenha cuidado ao instalá-los.",
  "Habilitar mimetização de Versão",
  "Versão a mimetizar",
  "Ação do botão ○",
  "Entrar",
  "Cancelar",
  "Recarrege o arquivo config.txt do taiHEN",
  "taiHEN config.txt foi recarregado com sucesso.",
  "Reiniciar o dispositivo",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by molecule
language_container_t language_english_gb = {
  "HENkaku Settings",
  "Enable PSN spoofing",
  "Enable Unsafe Homebrew",
  "Unsafe homebrews can damage your device permanently, if they are misused, misconfigured, or malicious. Please take caution when installing them.",
  "Enable Version Spoofing",
  "Spoofed Version",
  "○ Button Behaviour",
  "Enter",
  "Cancel",
  "Reload taiHEN config.txt",
  "taiHEN config.txt has been successfully reloaded.",
  "Reboot device",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

// by Chronicl3
language_container_t language_turkish = {
  "HENkaku Ayarları",
  "PSN spoofing Etkinleştir",
  "Yabancı Uygulamalara İzin Ver ",
  "Bilinmeyen kaynaklardan uygulama yüklerken dikkatli olunuz.Kötü niyetli,yanlış yapılandırılmış uygulamalar cihazınıza kalıcı zararlar verebilir.",
  "Versiyon Spoofing Etkinleştir",
  "Spoofed Versiyon",
  "○ Düğmesi Davranışı",
  "Gir",
  "İptal",
  "taiHEN config.txt'yi yeniden yükle",
  "taiHEN config.txt başarılı bir şekilde yeniden yüklendi.",
  "Cihazı yeniden başlat",
  "Content Downloader",
  "Unlink Memory Card",
  "ux0:id.dat has been deleted. This memory card can now be used on any Vita.",
  "Failed to mount ux0. Is a memory card inserted?",
};

#endif // LANGUAGE_HEADER
