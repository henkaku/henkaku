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
  "taiHEN config.txtのリロード",
  "taiHEN config.txtのリロードが完了しました。",
  "端末の再起動",
  "コンテンツ ダウンローダー",
  "メモリーカードとのリンクを解除する",
  "ux0:id.datが削除されました。メモリーカードはこのVita以外でも使用できるようになりました。",
  "ux0のマウントに失敗しました。メモリーカードは挿入されていますか？",
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

// by jokira & devnoname120 & CelesteBlue-dev & chronoss09 & francl35
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
  "Téléchargement de contenu",
  "Délier la carte mémoire",
  "ux0:id.dat a été supprimé. Cette carte mémoire peut être utilisée sur n'importe quelle Vita.",
  "Le montage de ux0 a échoué. Y a-t-il une carte mémoire insérée?",
};

// by EricWeichhart & iamn0tdev & WZ-JK
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
  "Descargador de Contenido",
  "Desvincular la Tarjeta de Memoria",
  "El ux0:id.dat ha sido eliminado. Esta tarjeta de memoria ahora se puede utilizar en cualquier Vita.",
  "Ha habido un fallo al montar el ux0. ¿Se ha insertado una tarjeta de memoria?",
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
  "Inhaltsdownloader",
  "Speicherkarte entkoppeln",
  "Der Dateieintrag ux0:id.dat wurde gelöscht. Diese Speicherkarte kann jetzt mit jeder Playstation Vita verwendet werden.",
  "Die Partition ux0: konnte nicht gemountet werden. Ist eine Speicherkarte eingesteckt?",
};

// by blackjack4it & Checcolino & Rufis01
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
  "Scarica contenuto",
  "Disassocia Memory Card",
  "ux0:id.dat è stato cancellato. Adesso questa Memory Card può essere usata su qualsiasi PS Vita.",
  "Impossibile montare ux0. La Memory Card è inserita?",
};

// by ConsoleHax & 2dook & jja2000
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
  "Inhoud downloader",
  "Memory Card loskoppelen",
  "ux0:id.dat is verwijderd. Deze Memory Card kan nu op elke Vita gebruikt worden.",
  "Aankoppelen van ux0 is mislukt. Is er wel een Memory Stick aanwezig?",
};

// by gnmmarechal & menelkir & bosshunter
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
  "Descarregar Conteúdo",
  "Desassociar Cartão de Memória",
  "ux0:id.dat foi eliminado. Este cartão de memória pode agora ser usado em qualquer Vita.",
  "Falha ao montar ux0. O cartão de memória está inserido?",
};

// by Tenek & MuskratDesman & dec0dOS
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
  "Загрузчик контента",
  "Отвязать карту памяти",
  "ux0:id.dat был удален. Эта карта памяти теперь может быть использована на любой PSVita.",
  "Ошибка при монтировании ux0. Проверьте, вставлена ли карта памяти",
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
  "컨텐츠 다운로더",
  "메모리 카드 언링크",
  "ux0:id.dat 파일이 삭제되었습니다. 이제 이 메모리 카드는 모든 비타에서 사용하실 수 있습니다.",
  "ux0에 마운트 할 수 없습니다. 메모리 카드가 올바르게 삽입되어 있습니까?",
};

// by iTZQing & FlexingTiger & magpte
language_container_t language_chinese_t = {
  "HENkaku設置",
  "啟用PSN偽裝",
  "啟用不安全自制軟件",
  "如果不安全自制軟件被誤用、配置出現錯誤或軟件本身是惡意程序，可能會永久性損壞你的設備。請謹慎安裝！",
  "啟用版本偽裝",
  "偽裝版本",
  "○鍵配置",
  "確定",
  "取消",
  "重新載入taiHEN config.txt",
  "重新載入taiHEN config.txt成功",
  "重啟設備",
  "下載內容管理器",
  "解除記憶卡關聯",
  "已刪除ux0:id.dat，現在這張記憶卡可應用到任意PSV上",
  "掛載ux0失敗，請檢查記憶卡是否插入",
};

// by iTZQing & FlexingTiger & magpte
language_container_t language_chinese_s = {
  "HENkaku设置",
  "启用PSN伪装",
  "启用不安全自制软件",
  "如果不安全自制软件被误用、配置出现错误或软件本身是恶意程序，可能会永久性损坏你的设备。请谨慎安装！",
  "启用版本伪装",
  "伪装版本",
  "○键配置",
  "确定",
  "取消",
  "重新载入taiHEN config.txt",
  "重新载入taiHEN config.txt成功",
  "重启设备",
  "下载内容管理器",
  "解除存储卡关联",
  "已删除ux0:id.dat，现在这张存储卡可用在任意PSV上",
  "挂载ux0失败，请检查存储卡是否插入",
};

// by v5000a & poppamies
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
  "Sisällönlataaja",
  "Irrota muistikortti",
  "ux0:id.dat on poistettu. Voit nyt käyttää tätä muistikorttia millä tahansa Vitalla.",
  "ux0:n liittäminen epäonnistui. Onko muistikortti laitteessa?",
};

// by MrOrbital & GizmoTheGreen & Wassburgare
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
  "Innehållsnedladdare",
  "Avlänka minneskort",
  "ux0:id.dat har tagits bort. Det här minneskortet kan nu användas i valfri Vita.",
  "Misslyckades med att montera ux0. Har du satt i ett minneskort?",
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
  "Fjern link til hukommelseskort",
  "ux0:id.dat er blevet slettet. Dette hukommelseskort kan nu bruges på enhver Vita.",
  "Kunne ikke gemme ux0. Er der indsat et hukommelseskort?",
};

// by baniel105 & irchagaming & Promises
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
  "Innholds Nedlaster",
  "Lås opp Minnekort",
  "ux0:id.dat er slettet. Dette minnekortet kan brukest på hvilken som helst Vita.",
  "Klarte ikkje montere av ux0. Har du satt i eit minnekort?",
};

// by Grzybojad & szczuru & ItsYogSothoth & szczuru
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
  "Pobieranie zawartości",
  "Odłącz kartę pamięci",
  "Plik ux0:id.dat został usunięty z karty pamięci. Można jej teraz używać z każdym systemem PS Vita",
  "Błąd podczas montowania partycji ux0, czy karta pamięci jest włożona?",
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
  "Baixar Conteúdo",
  "Desassociar o Cartão de Memória",
  "ux0:id.dat foi deletado. Agora este cartão de memória pode ser usado em qualquer Vita.",
  "Falha ao montar ux0. O cartão de memória está inserido?",
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
  "İçerik İndiricisi",
  "Hafıza Kartı Bağlantısını Kaldır",
  "ux0:id.dat silindi.Bu hafıza kartı şuan herhangi bir vita da kullanılabilir",
  "ux0 dizin okuma başarısız oldu.Hafıza kartınız takılı mı?",
};

#endif // LANGUAGE_HEADER
