#pragma once
#include <stddef.h>
#include <stdint.h>

namespace brico {
enum class Language : uint8_t { Spanish, English, German, French, Italian, Count };
constexpr size_t kLanguageCount = size_t(Language::Count);
constexpr const char* kLanguageCodes[] = {"es", "en", "de", "fr", "it"};
constexpr const char* kLanguageNames[] = {"Español", "English", "Deutsch", "Français", "Italiano"};

// One catalog drives both the TFT and the phone UI.
#define BRICO_TRANSLATIONS(X) \
X(Receive,"Recibir","Receive","Empfangen","Recevoir","Ricevere") \
X(Send,"Enviar","Send","Senden","Envoyer","Inviare") \
X(Config,"Ajustes","Settings","Einstellungen","Réglages","Impostazioni") \
X(Koch,"Koch","Koch","Koch","Koch","Koch") \
X(Letters,"Letras","Letters","Buchstaben","Lettres","Lettere") \
X(Words,"Palabras","Words","Wörter","Mots","Parole") \
X(Numbers,"Números","Numbers","Zahlen","Chiffres","Numeri") \
X(Mixed,"Mixto","Mixed","Gemischt","Mixte","Misto") \
X(Card,"Tarjeta SD","SD card","SD-Karte","Carte SD","Scheda SD") \
X(Qso,"QSO","QSO","QSO","QSO","QSO") \
X(Calls,"Indicativos","Call signs","Rufzeichen","Indicatifs","Nominativi") \
X(Practice,"Práctica","Practice","Üben","Pratique","Pratica") \
X(CopyOne,"Copiar 1","Copy 1","Kopieren 1","Copier 1","Copia 1") \
X(CopyTwo,"Copiar 2","Copy 2","Kopieren 2","Copier 2","Copia 2") \
X(CopyWord,"Copiar palabra","Copy word","Wort kopieren","Copier mot","Copia parola") \
X(CopyCall,"Copiar indic.","Copy call","Rufz. kopieren","Copier indic.","Copia nomin.") \
X(Flash,"Tarjetas","Flash cards","Lernkarten","Cartes","Schede") \
X(HeadCopy,"Copia mental","Head copy","Gehörlesen","Copie mentale","Copia mentale") \
X(Network,"Red CW","CW room","CW-Runde","Salon CW","Rete CW") \
X(Echo,"Entrenador eco","Echo trainer","Echo-Trainer","Entraîneur écho","Allenatore eco") \
X(Adaptive,"Adaptativo","Adaptive","Adaptiv","Adaptatif","Adattivo") \
X(Memories,"Memorias","Memories","Speicher","Mémoires","Memorie") \
X(Statistics,"Estadísticas","Statistics","Statistik","Statistiques","Statistiche") \
X(Speed,"Velocidad","Speed","Tempo","Vitesse","Velocità") \
X(Farnsworth,"Farnsworth","Farnsworth","Farnsworth","Farnsworth","Farnsworth") \
X(Pitch,"Tono","Pitch","Tonhöhe","Tonalité","Tono") \
X(KeyMode,"Manipulador","Key mode","Tastmodus","Manipulateur","Tasto") \
X(Callsign,"Indicativo","Call sign","Rufzeichen","Indicatif","Nominativo") \
X(Brightness,"Brillo","Brightness","Helligkeit","Luminosité","Luminosità") \
X(KochLevel,"Nivel Koch","Koch lesson","Koch-Lektion","Leçon Koch","Lezione Koch") \
X(GroupSize,"Largo grupo","Group size","Gruppenlänge","Taille groupe","Lung. gruppo") \
X(Reset,"Restaurar","Defaults","Standardwerte","Réinitialiser","Ripristina") \
X(UiLanguage,"Idioma pantalla","UI language","Menüsprache","Langue menus","Lingua menu") \
X(WordLanguage,"Idioma palabras","Word language","Wortsprache","Langue mots","Lingua parole") \
X(Sequence,"Secuencia Koch","Koch sequence","Koch-Reihenfolge","Séquence Koch","Sequenza Koch") \
X(CustomSet,"Conjunto propio","Custom set","Eigene Zeichen","Jeu personnel","Set personale") \
X(WordGap,"Pausa palabras","Word gap","Wortabstand","Espace mots","Spazio parole") \
X(Back,"< Atrás","< Back","< Zurück","< Retour","< Indietro") \
X(Save,"Guardar","Save","Speichern","Sauver","Salva") \
X(Delete,"Borrar","Delete","Löschen","Effacer","Cancella") \
X(Edit,"Editar","Edit","Ändern","Modifier","Modifica") \
X(Play,"Escuchar","Play","Abspielen","Écouter","Ascolta") \
X(Next,"Sig.","Next","Weiter","Suiv.","Avanti") \
X(Yes,"Sí","Yes","Ja","Oui","Sì") \
X(No,"No","No","Nein","Non","No") \
X(Straight,"Llave recta","Straight key","Handtaste","Pioche","Tasto verticale") \
X(Pending,"Pendientes","Pending","Wartend","En attente","In attesa") \
X(NetworkError,"Error de red; ver Serial","Network error; see Serial","Netzfehler; siehe Serial","Erreur réseau; voir Serial","Errore rete; vedi Serial") \
X(AudioError,"Error de audio; ver Serial","Audio error; see Serial","Audiofehler; siehe Serial","Erreur audio; voir Serial","Errore audio; vedi Serial") \
X(SaveError,"Error al guardar","Cannot save","Speichern fehlgeschlagen","Échec enregistrement","Salvataggio fallito") \
X(PlayError,"Error de mensaje o audio","Message or audio error","Nachricht-/Audiofehler","Erreur message ou audio","Errore messaggio o audio") \
X(TextAudio,"Texto + audio","Text + audio","Text + Ton","Texte + audio","Testo + audio") \
X(MorseOnly,"Solo Morse","Morse only","Nur Morse","Morse seul","Solo Morse") \
X(Password,"Clave","Password","Passwort","Mot de passe","Password") \
X(Reveal,"OK: revelar respuesta","OK: reveal answer","OK: Antwort zeigen","OK : voir réponse","OK: mostra risposta") \
X(AnswerPrompt,"Repite; pausa para evaluar","Repeat; pause to score","Wiederholen; Pause wertet","Répétez ; pause pour noter","Ripeti; pausa per valutare") \
X(TestTone,"DIT: probar tono","DIT: test tone","DIT: Ton testen","DIT : tester le son","DIT: prova tono") \
X(ResetQuestion,"Restaurar ajustes del tutor?","Reset tutor settings?","Tutor-Einstellungen zurück?","Réinitialiser les réglages ?","Ripristinare impostazioni?") \
X(ResetNote,"No borra memorias ni estadísticas","Keeps memories and statistics","Speicher und Statistik bleiben","Garde mémoires et statistiques","Conserva memorie e statistiche") \
X(NoFiles,"Sin TXT en la raíz","No root TXT files","Keine TXT im Hauptordner","Aucun TXT à la racine","Nessun TXT nella radice") \
X(NoCard,"SD no disponible","SD unavailable","SD nicht verfügbar","SD indisponible","SD non disponibile") \
X(FileError,"Error al abrir archivo","Cannot open file","Datei nicht lesbar","Fichier inaccessible","File non accessibile") \
X(PracticePrompt,"Usa DIT/DAH o la llave. ","Use DIT/DAH or the key. ","DIT/DAH oder Taste nutzen. ","Utilisez DIT/DAH ou la clé. ","Usa DIT/DAH o il tasto. ") \
X(NewSpeed,"Velocidad: siguiente envío","Speed: next transmission","Tempo: nächste Sendung","Vitesse : prochain envoi","Velocità: prossimo invio") \
X(Queued,"En cola; entrega no confirmada","Queued; delivery unconfirmed","Wartend; Empfang unbestätigt","En file ; réception non confirmée","In coda; consegna non confermata") \
X(TxError,"No enviado: red, texto o cola","Not sent: network, text or queue","Nicht gesendet: Netz/Text/Puffer","Non envoyé : réseau/texte/file","Non inviato: rete/testo/coda") \
X(InvalidCall,"Indicativo inválido","Invalid call sign","Ungültiges Rufzeichen","Indicatif invalide","Nominativo non valido") \
X(Correct,"Correcto","Correct","Richtig","Correct","Corretto") \
X(Expected,"Era","Expected","Erwartet","Attendu","Atteso") \
X(TooLong,"Respuesta demasiado larga","Answer too long","Antwort zu lang","Réponse trop longue","Risposta troppo lunga") \
X(EndFile,"Fin de archivo","End of file","Dateiende","Fin du fichier","Fine del file") \
X(DecodeSaved,"Modo RX guardado","RX mode saved","RX-Modus gespeichert","Mode RX enregistré","Modo RX salvato") \
X(EmptyMemory,"Memoria vacía; pulsa Editar","Empty memory; choose Edit","Speicher leer; Bearbeiten","Mémoire vide ; Modifier","Memoria vuota; Modifica") \
X(InvalidSet,"Usa 2-40 símbolos distintos","Use 2-40 unique symbols","2-40 verschiedene Zeichen","Utilisez 2-40 signes distincts","Usa 2-40 simboli distinti") \
X(NoStats,"Sin respuestas evaluadas","No scored answers","Noch keine Auswertung","Aucune réponse évaluée","Nessuna risposta valutata") \
X(Attempts,"Intentos","Attempts","Versuche","Essais","Tentativi") \
X(Errors,"Errores","Errors","Fehler","Erreurs","Errori") \
X(Accuracy,"Aciertos","Accuracy","Trefferquote","Précision","Precisione") \
X(SequenceBrico,"Bricohams","Bricohams","Bricohams","Bricohams","Bricohams") \
X(SequenceLetters,"Letras primero","Letters first","Buchstaben zuerst","Lettres d'abord","Prima lettere") \
X(SequenceDigits,"Números primero","Numbers first","Zahlen zuerst","Chiffres d'abord","Prima numeri") \
X(WebIntro,"Red local WiFi/ESP-NOW, canal 1. Sin Internet, sin RF de radioaficionado, sin retransmisión mesh. No garantiza entrega ni confidencialidad.","Local WiFi/ESP-NOW, channel 1. No Internet, amateur RF or mesh relay. Delivery and confidentiality are not guaranteed.","Lokales WiFi/ESP-NOW, Kanal 1. Kein Internet, Amateurfunk oder Mesh-Relay. Keine Empfangs- oder Vertraulichkeitsgarantie.","Réseau local WiFi/ESP-NOW, canal 1. Sans Internet, RF radioamateur ni relais mesh. Livraison et confidentialité non garanties.","Rete locale WiFi/ESP-NOW, canale 1. Senza Internet, RF radioamatoriale o relay mesh. Consegna e riservatezza non garantite.") \
X(Message,"Mensaje (hasta 120 caracteres)","Message (up to 120 characters)","Nachricht (bis 120 Zeichen)","Message (120 caractères max.)","Messaggio (max. 120 caratteri)") \
X(SendAll,"Enviar a todos","Send to all","An alle senden","Envoyer à tous","Invia a tutti") \
X(DecodeLabel,"Mostrar texto RX decodificado","Show decoded RX text","Dekodierten RX-Text zeigen","Afficher le texte RX décodé","Mostra testo RX decodificato") \
X(Hidden,"Texto oculto para practicar","Text hidden for practice","Text zum Üben ausgeblendet","Texte masqué pour pratiquer","Testo nascosto per esercitarsi") \
X(Drops,"Descartes","Drops","Verworfen","Rejets","Scarti") \
X(InvalidPackets,"Paquetes inválidos","Invalid packets","Ungültige Pakete","Paquets invalides","Pacchetti non validi") \
X(TxFailures,"Fallos TX","TX failures","TX-Fehler","Échecs TX","Errori TX") \
X(Offline,"No se puede leer el dispositivo","Cannot read device state","Gerät nicht erreichbar","Appareil inaccessible","Dispositivo non raggiungibile") \
X(Recovered,"Conexión recuperada","Connection restored","Verbindung wiederhergestellt","Connexion rétablie","Connessione ripristinata") \
X(Unauthorized,"Sesión no autorizada; recarga","Unauthorized session; reload","Sitzung ungültig; neu laden","Session non autorisée ; rechargez","Sessione non autorizzata; ricarica") \
X(InvalidInput,"Entrada no válida","Invalid input","Ungültige Eingabe","Entrée invalide","Dati non validi") \
X(Saved,"Guardado","Saved","Gespeichert","Enregistré","Salvato") \
X(MorseNote,"Los acentos se normalizan a Morse básico. Las ligaduras se expanden. No se envían caracteres extendidos.","Accents normalize to basic Morse. Ligatures expand. No extended characters are transmitted.","Akzente werden für einfaches Morse normalisiert. Ligaturen werden erweitert. Keine erweiterten Morsezeichen.","Accents normalisés en Morse de base. Les ligatures sont développées. Aucun caractère étendu transmis.","Accenti normalizzati in Morse base. Le legature vengono espanse. Nessun carattere esteso trasmesso.") \
X(Calibrate,"Toca las tres cruces","Touch the three crosses","Die drei Kreuze berühren","Touchez les trois croix","Tocca le tre croci")

enum class Text {
#define BRICO_ENUM(id,es,en,de,fr,it) id,
  BRICO_TRANSLATIONS(BRICO_ENUM)
#undef BRICO_ENUM
  Count
};
struct Translation { const char* key; const char* values[kLanguageCount]; };
constexpr Translation kTranslations[] = {
#define BRICO_ROW(id,es,en,de,fr,it) {#id, {es,en,de,fr,it}},
  BRICO_TRANSLATIONS(BRICO_ROW)
#undef BRICO_ROW
};
#undef BRICO_TRANSLATIONS
static_assert(sizeof(kTranslations) / sizeof(*kTranslations) == size_t(Text::Count), "Complete catalog");
inline bool validLanguage(int index) { return index >= 0 && size_t(index) < kLanguageCount; }
inline const char* translate(Text id, Language language) {
  return kTranslations[size_t(id)].values[size_t(language)];
}
}
