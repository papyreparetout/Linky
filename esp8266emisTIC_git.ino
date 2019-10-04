/*
 
 Simulateur compteur monophasé Linky en mode historique par envoi de caractere
 sur une liaison Wifi avec un ESP8266

 L'intensité instantanée varie périodiquement 
 entre 0 et 120% de l'intensite souscrite
 Le programme permet egalement de simuler le passage de heures pleines à heures creuses
 et incremente les index correspondants
 
 cree par J. ROGUIN pour Artilect Fablab 10/2017
 */
 
#include <ESP8266WiFi.h>

unsigned long tdeb, duree, interv, bascultar;
char check;
String texte = "";
String textcheck;
char sepdonn = 0x20;
char sepgroup = 0x0D;
char debgroup = 0x0A;
String debtrame = "ADCO";
String numcompt = "021675649359";
// String choixtarif = "BASE"; // option tarifaire soit "BASE" soit "HC.."
String choixtarif = "HC..";
int choixisous = 30;  // valeur de l intensite souscrite (= Psouscrite/200)
char strixisous[2];
char indexc[9];
// String ptec = "TH.."; // "TH.." pour l'option base sinon "HP.." ou "HC.."
String ptec[3] = {"TH..","HP..","HC.."}; // "TH.." pour l'option base sinon "HP.." ou "HC.."
char intensi[3];
String papp = "PAPP";
char pappstr[5];
int longbuff;
uint16_t intnum = 0;  
uint16_t pappnum,val;
uint32_t indexdeb = 400000; // valeur arbitraire heures pleines ou base
uint32_t indexdhc = 115000; // index debut heures creuses
uint32_t indexnum, indexnumc,indexnump;
// attention au dimensionnemment du buffer qui doit être supérieur au max de longueur de texte
byte bufenvoi[250];					
boolean fin = false;
boolean dps = false;
boolean tbase = false;
boolean bascul = false;
float energ,valint,energp,energc;
// pour generer un client Wifi
const char* ssid     = "xxxxxxxxxx";      // SSID
const char* password = "xxxxxxxxxxx";      // Password de la box
const char* host = "192.168.1.xx";  // IP serveur - Server IP
const int   port = xx;            // Port serveur - Server Port

void setup() {
// Ouvre le port serie vers la console
  Serial.begin(115200);

// demarrage du client
 Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
 
if (choixtarif == "BASE") { tbase = true; }
tdeb = millis();
// intervalle entre deux envois de trame en  ms
interv = 1000;
// duree pendant laquelle les trames sont envoyees en ms 
 duree = 12000000;
// instant du basculement de tarif en ms, à caler par rapport à la duree de simulation!
bascultar = 20000;
 
 energ = 0; // initialisation calcul energie pour l'index
 energp = 0;
 energc = 0;
}

void loop() { 
  
  while ((millis()-tdeb) < duree) {
  
    WiFiClient client;
    if (!client.connect(host, port)) {
      Serial.println("connection failed");
      return;
		}
  
  if((millis()-tdeb) >= bascultar) {
	bascul = true;
	}

//
// calcul des valeurs numériques variables
//
// 
/*val = analogRead(A0);  // lecture entree analogique valeur de IINST
valint = float(val)*12.0/10240.0*float(choixisous);  // conversion de l entree ana en A (1024 = 120% de ISOUSC)
intnum = int(valint);
// Serial.println(val+","+String(valint)+","+intnum);
*/
intnum = ((millis() % 1000) % 12)* choixisous/10 ;
Serial.println(" Intensité"+String(intnum));
uint16_t intnum2 = intnum;
pappnum = intnum2*200;
if (tbase) {
	energ = energ + pappnum*intnum2/(5*3600);
	indexnum = indexdeb + long(energ);
	}
else {
		if (bascul) {
			energc = energc + pappnum*intnum2/(5*3600);
			indexnumc = indexdhc + long(energc);
			}
		else {
			energp = energp + pappnum*intnum2/(5*3600);
			indexnump = indexdeb + long(energp);
			indexnumc = indexdhc;
			}
	}

if(intnum >= choixisous) {dps = true;}// depassement PS
  else {dps = false;}
// 
// construction de la trame
// debut de trame
//
texte = "";
texte = char(0x02);
texte = texte+char(0x0A);
// premier groupe: numero compteur
texte = texte + debtrame + sepdonn + numcompt + sepdonn + char(0x40)+sepgroup ;
//  groupe option tarifaire
  texte = texte + debgroup;
  textcheck = String("OPTARIF") + sepdonn + choixtarif+ sepdonn; 
// pour calcul checksum
  check = checksum(textcheck);
  texte = texte + textcheck + check + sepgroup;
// groupe intensité souscrite
  texte = texte + debgroup;
  sprintf(strixisous,"%2d",choixisous);
  textcheck = String("ISOUSC") + sepdonn + strixisous + sepdonn;
  check = checksum(textcheck);
  texte = texte + textcheck + check + sepgroup;
// groupe index 
  texte = texte + debgroup;
  if (tbase) {
		sprintf(indexc,"%09ld",indexnum);
		textcheck = choixtarif + sepdonn + indexc + sepdonn;
		check = checksum(textcheck);
		texte = texte + textcheck + check + sepgroup;
		}
	else {
// index heures creuses
		sprintf(indexc,"%09ld",indexnumc);
		textcheck = String("HCHC") + sepdonn + indexc + sepdonn;
		check = checksum(textcheck);
		texte = texte + textcheck + check + sepgroup;
// index heures pleines
    texte = texte + debgroup;
   	sprintf(indexc,"%09ld",indexnump);
		textcheck = String("HCHP") + sepdonn + indexc + sepdonn;
		check = checksum(textcheck);
		texte = texte + textcheck + check + sepgroup;
		}
// groupe période tarifaire en cours
  texte = texte + debgroup;
  if (tbase) {textcheck = String("PTEC") + sepdonn + ptec[0] + sepdonn;}
	else {
		if (bascul) {textcheck = String("PTEC") + sepdonn + ptec[2] + sepdonn;}
			else {textcheck = String("PTEC") + sepdonn + ptec[1] + sepdonn;}
		}
  check = checksum(textcheck);
  texte = texte + textcheck + check + sepgroup;
// groupe intensité instantanée
  sprintf(intensi,"%03d",intnum2);
  texte = texte + debgroup;
  textcheck = String("IINST") + sepdonn + intensi + sepdonn;
  check = checksum(textcheck);
  texte = texte + textcheck + check + sepgroup;
// groupe depassement PS 
	if (dps) {
	texte = texte + debgroup;
	textcheck = String("ADPS") + sepdonn + intensi + sepdonn;
	check = checksum(textcheck);
	texte = texte + textcheck + check + sepgroup;
	}
// groupe IMAX
  texte = texte + debgroup;
  textcheck = String("IMAX") + sepdonn + char('0')+ char('9')+char('0') + sepdonn;
  check = checksum(textcheck);
  texte = texte + textcheck + check + sepgroup; 
// groupe puissance appelée
  sprintf(pappstr,"%05d",pappnum);
  texte = texte + debgroup;
  textcheck = papp + sepdonn + pappstr + sepdonn;
  check = checksum(textcheck);
  texte = texte + textcheck + check + sepgroup;
// groupe HCHP
  texte = texte + debgroup;
  textcheck = String("HHPHC") + sepdonn + char('A') + sepdonn;
  check = checksum(textcheck);
  texte = texte + textcheck + check + sepgroup;
// groupe mot d'état compteur
  texte = texte + debgroup;
  textcheck = String("MOTDETAT") + sepdonn + String("000000") + sepdonn;
  check = checksum(textcheck);
  texte = texte + textcheck + check;  
// fin de trame
texte= texte + char(0x0D)+ char(0x03);
//
//  envoi de la trame sur la liaison serie
//
longbuff = texte.length();
for (int i=0;i<(longbuff);i++){
    bufenvoi[i]= texte[i];
  }
 
// Envoi donnees au serveur 
    client.write(bufenvoi,longbuff);


//       Serial.write(bufenvoi,153);
       Serial.println(texte);
//     digitalWrite(13,LOW);
// intervalle entre deux envois de trame	   
       delay(interv);
    
  }
  if (!fin) { // mySerial.println("fin d'envoi");
        Serial.println("fin d'envoi");
        fin = true;}
}

// routines
char checksum(String textacheck) // routine de calcul de checksum
{
char Controle=0x00;
for (int i=0;i<(textacheck.length()-1);i++){
    Controle += textacheck[i];
	} 
Controle = (Controle & 0x3F) + 0x20;
return Controle;
}
