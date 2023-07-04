#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include "WiFi.h"
#include <EEPROM.h>
#include "ESPAsyncWebServer.h"
#include <Preferences.h>
#include <AsyncTCP.h>


AsyncWebServer server(80);
bool a = LOW; 
bool b = LOW; 
bool c = LOW; 
bool d = LOW; 
bool signupOK = false; 
const char* PARAM_INPUT_1 = "cmd";
const char* PARAM_INPUT_2 = "date";
const char* PARAM_INPUT_3 = "commande";
int idscan;

IPAddress local_IP(192,168,16,45);
IPAddress gateway(192,168,16,9);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional
IPAddress ip;


#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// Pour UNO et autres sans série matérielle, nous devons utiliser le logiciel de série...
// Configure le port série pour utiliser le logiciel de série..
SoftwareSerial mySerial(Finger_Tx2, Finger_Rx2);

#else
// Sur Leonardo/M0/etc, et autres avec série matérielle, utilise la série matérielle !
// #0 est le fil vert, #1 est le fil blanc
#define mySerial Serial2

#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
// Définir la taille de l'espace mémoire à allouer pour l'EEPROM
#define EEPROM_SIZE 512// Structure pour stocker les données
struct Data {
 uint8_t empreintesEnregistrees[128];
  uint8_t nbEmpreintesEnregistrees = 0;
};

//Variable pour envoyer des données à l'application
String global_var = "-1";

//Variable pour recevoir des données de l'application
String take_value = "-1";

//int p = -1;



void setup()
{
  // Initialiser l'EEPROM avec la taille spécifiée
  EEPROM.begin(EEPROM_SIZE);
  
  Serial.begin(9600);
  while (!Serial);  // Pour Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nEnregistrement de l'empreinte digitale avec le capteur Adafruit");

  // Définir le débit de données pour le port série du capteur
  finger.begin(57600);

  if (finger.verifyPassword())
  {
    Serial.println("Capteur d'empreintes digitales trouvé !");
  }
  else
  {
    Serial.println("Capteur d'empreintes digitales non trouvé :(");
    while (1) { delay(1); }
  }

  WiFi.begin("CPE_R0516_6B7A", "41004418");
  //WiFi.begin("WIRELESS-CCP", "OnlyForCCP2021");
  //WiFi.begin("Centre De Calcul Annexe", "OnlyForCalculUsers2021");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Local IP address = ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
    }
    Serial.println(inputMessage);
    if (inputMessage == "a") {
      a = HIGH;
      b = LOW;
      c = LOW;
      d = LOW;
      delay(50);
      request->send(200, "text/plain", String(global_var));
    } else if (inputMessage == "b") {
      a = LOW;
      b = HIGH;
      c = LOW;
      d = LOW;
      delay(50);
      request->send(200, "text/plain", String(global_var));
    } else if (inputMessage == "c") {
      a = LOW;
      b = LOW;
      c = HIGH;
      d = LOW;
      delay(50);
      request->send(200, "text/plain", String(global_var));
    } else if (inputMessage == "d") {
      a = LOW;
      b = LOW;
      c = LOW;
      d = HIGH;
      delay(50);
      request->send(200, "text/plain", String(global_var));
    } else if (inputMessage == "e") {
      request->send(200, "text/plain", String(String(global_var)));
      idscan = 0;
    }
  });

  
  server.on("/", HTTP_POST, handlePostRequest);
  
  Serial.println("Server started");
  server.begin();
}
int p;

void loop(){
  
if(take_value == "enroll"){
  //j'enclenche l'enregistrement
      //global_var = String(-1);
      //Serial.println("Valeur changée");
      verifierEmpreinte();
    
    }else if(take_value == "go"){
        enregistrerEmpreinte();
        
        Serial.println("000000000000000000000000000000000000000");      
      take_value = "-1";
    }
      else if(take_value == "-1"){
        verifierEmpreinte();
    
        Serial.println("##############################");

    } else if(take_value == "update"){
      Serial.println("Okkkkk");
      delay(5000);
      global_var = String(-1);
      take_value == "-1";
      
     Serial.println("Goooofo");
    
    }else if(take_value == "0"){
      Serial.println("Perfect Action");
    }else{
      Serial.println("Je suis la");
      int Id = take_value.toInt();
      if(Id>0 && Id<= 127){
       supprimerEmpreinte(Id); 
      }else{
        Serial.println("Aucune suppression");  
      }
      
      take_value = "0";
    }  

//supprimerToutesEmpreintes();
    
}

void writeDataToEEPROM(const Data& data) {
  // Calculer l'adresse de départ dans l'EEPROM
  int address = 0;

  // Écrire les données dans l'EEPROM
  EEPROM.put(address, data);

  // Enregistrer les modifications
  EEPROM.commit();
}

Data readDataFromEEPROM() {
  // Calculer l'adresse de départ dans l'EEPROM
  int address = 0;

  // Lire les données depuis l'EEPROM
  Data data;
  EEPROM.get(address, data);

  return data;
}

// ---------------------------------------------------Fonction qui fournir un ID Disponible du capteur---------------------------------------
int obtenirIdDisponible()
{
    Data data;
    EEPROM.get(0, data);
  
  // Vérifier si tous les ID sont utilisés
  if (data.nbEmpreintesEnregistrees >= 127)
  {
    return -2; // Tous les ID sont utilisés
  }

  // Rechercher un ID disponible
  for (int id = 1; id <= 127; id++)
  {
    bool idUtilise = false;

    // Vérifier si l'ID est déjà utilisé
    for (int i = 0; i < data.nbEmpreintesEnregistrees; i++)
    {
      if (data.empreintesEnregistrees[i] == id)
      {
        idUtilise = true;
        break;
      }
    }

    if (!idUtilise)
    {
      return id; // ID disponible trouvé
    }
  }

  // Par précaution, si aucun ID disponible n'a été trouvé
  return -1;
}


// ------------------------------------Fonction de vérification de l'existence d'une empreinte-----------------------------------------------

void verifierEmpreinte()
{
  Serial.println("Prêt à vérifier une empreinte !");
  Serial.println("Placez votre doigt sur le capteur...");

  obtenirEmpreinteVerification();
}

int obtenirEmpreinteVerification()
{
  int p = -1;

  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();

    switch (p)
    {
      case FINGERPRINT_OK:
        Serial.println("Image capturée");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println("Aucun doigt détecté. Veuillez réessayer.");
        global_var = String(150); // L'application doit demander à l'utilisateur de placer son doigt
        delay(500);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Erreur de communication");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Erreur d'imagerie");
        break;
      default:
        Serial.println("Erreur inconnue");
        break;
    }
  }

  p = finger.image2Tz();

  if (p != FINGERPRINT_OK)
  {
    Serial.println("Erreur de conversion de l'image. Veuillez réessayer.");
    return -1;
  }

  p = finger.fingerFastSearch();

  if (p == FINGERPRINT_OK)
  {
    uint8_t match_id = finger.fingerID;
    uint16_t match_score = finger.confidence;
    global_var = String(match_id);
    Serial.print("Correspondance de l'empreinte avec l'ID #");
    Serial.print(match_id);
    p = 0;
        while (p != FINGERPRINT_NOFINGER)
        {
          p = finger.getImage();
        }
    
    Serial.print(" (Score : ");
    Serial.print(match_score);
    //Serial.println(")");
    
    //delay(15000);
    Serial.println(global_var);
    return 1;
  }
  else
  {
    global_var = String(151);
    Serial.println("Empreinte non trouvée");
    Serial.println(global_var);
    p = 0;
        while (p != FINGERPRINT_NOFINGER)
        {
          p = finger.getImage();
        }
    Serial.println("------------------------------------");
    
    return -1;
  }
}

// --------------------------------------------Fonction d'enregistrement d'une empreinte-----------------------------------------------------
void enregistrerEmpreinte() {


  int id = obtenirIdDisponible();
  
 /* if (finger.deleteModel(id) == FINGERPRINT_OK) {
    Serial.println("Empreinte existante supprimée.");
  }
*/
  Serial.print("Enregistrement de l'empreinte avec l'ID #");
  Serial.println(id);
  Serial.println("Placez votre doigt sur le capteur...");

  if (getFingerprintEnroll(id)) {
    // Ajoute l'ID dans le tableau des empreintes enregistrées
    Data data;
        EEPROM.get(0, data);
        data.empreintesEnregistrees[data.nbEmpreintesEnregistrees] = id;
        data.nbEmpreintesEnregistrees++;
        EEPROM.put(0, data);
        EEPROM.commit();
        Serial.println(String(data.empreintesEnregistrees[data.nbEmpreintesEnregistrees]));
        Serial.println(String(data.nbEmpreintesEnregistrees));
    Serial.println("Enregistrement de l'empreinte terminé.");
  } else {
    Serial.println("Échec de l'enregistrement de l'empreinte.");
  }
}


bool getFingerprintEnroll(int id) {

int p = -1;
Serial.print("En attente d'un doigt valide pour l'enregistrement en tant que n°"); Serial.println(id);
while (p != FINGERPRINT_OK) {
p = finger.getImage();
switch (p) {
case FINGERPRINT_OK:
Serial.println("Image capturée");
break;
case FINGERPRINT_NOFINGER:
Serial.println(".");
break;
case FINGERPRINT_PACKETRECIEVEERR:
Serial.println("Erreur de communication");
break;
case FINGERPRINT_IMAGEFAIL:
Serial.println("Erreur de capture d'image");
break;
default:
Serial.println("Erreur inconnue");
break;
}
}

// OK, succès !

p = finger.image2Tz(1);
switch (p) {
case FINGERPRINT_OK:
Serial.println("Image convertie");
break;
case FINGERPRINT_IMAGEMESS:
Serial.println("Image trop floue");
return false;
case FINGERPRINT_PACKETRECIEVEERR:
Serial.println("Erreur de communication");
return false;
case FINGERPRINT_FEATUREFAIL:
Serial.println("Impossible de trouver les caractéristiques de l'empreinte");
return false;
case FINGERPRINT_INVALIDIMAGE:
Serial.println("Impossible de trouver les caractéristiques de l'empreinte");
return false;
default:
Serial.println("Erreur inconnue");
return false;
}
global_var = String(id);
Serial.println("Retirez le doigt");

delay(2000);
p = 0;
while (p != FINGERPRINT_NOFINGER) {
p = finger.getImage();
}
Serial.print("ID "); Serial.println(id);

p = -1;
Serial.println("Replacez le même doigt");
while (p != FINGERPRINT_OK) {
p = finger.getImage();
switch (p) {
case FINGERPRINT_OK:
Serial.println("Image capturée");
break;
case FINGERPRINT_NOFINGER:
Serial.print(".");
break;
case FINGERPRINT_PACKETRECIEVEERR:
Serial.println("Erreur de communication");
break;
case FINGERPRINT_IMAGEFAIL:
Serial.println("Erreur de capture d'image");
break;
default:
Serial.println("Erreur inconnue");
break;
}
}

// OK, succès !

p = finger.image2Tz(2);
switch (p) {
case FINGERPRINT_OK:
Serial.println("Image convertie");
break;
case FINGERPRINT_IMAGEMESS:
Serial.println("Image trop floue");
return false;
case FINGERPRINT_PACKETRECIEVEERR:
Serial.println("Erreur de communication");
return false;
case FINGERPRINT_FEATUREFAIL:
Serial.println("Impossible de trouver les caractéristiques de l'empreinte");
return false;
case FINGERPRINT_INVALIDIMAGE:
Serial.println("Impossible de trouver les caractéristiques de l'empreinte");
return false;
default:
Serial.println("Erreur inconnue");
return false;
}

// OK, convertie !
Serial.print("Création du modèle pour le n°"); Serial.println(id);

p = finger.createModel();
if (p == FINGERPRINT_OK) {
Serial.println("Correspondance des empreintes !");
} else if (p == FINGERPRINT_PACKETRECIEVEERR) {
Serial.println("Erreur de communication");
return false;
} else if (p == FINGERPRINT_ENROLLMISMATCH) {
Serial.println("Les empreintes ne correspondent pas");
global_var = String(151);
Serial.println("Retirez le doigt.Echec");
delay(2000);
p = 0;
while (p != FINGERPRINT_NOFINGER) {
p = finger.getImage();
}
return false;
} else {
Serial.println("Erreur inconnue");
return false;
}

Serial.print("ID "); Serial.println(id);
p = finger.storeModel(id);
if (p == FINGERPRINT_OK) {
Serial.println("Enregistré Avec Succes!");
global_var = String(1111);
Serial.println("Retire");
delay(2000);
p = 0;
while (p != FINGERPRINT_NOFINGER) {
p = finger.getImage();
}
} else if (p == FINGERPRINT_PACKETRECIEVEERR) {
Serial.println("Erreur de communication");
return false;
} else if (p == FINGERPRINT_BADLOCATION) {
Serial.println("Impossible d'enregistrer à cet emplacement");
return false;
} else if (p == FINGERPRINT_FLASHERR) {
Serial.println("Erreur d'écriture sur la mémoire flash");
return false;
} else {
Serial.println("Erreur inconnue");
return false;
}

return true;
}

bool obtenirEmpreinteEnregistrement(uint8_t id)
{
  int p = -1;

  while (p != FINGERPRINT_OK)
  {
    p = finger.getImage();

    switch (p)
    {
      case FINGERPRINT_OK:
        Serial.println("Image capturée");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println("Aucun doigt détecté. Veuillez réessayer.");
        delay(500);
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Erreur de communication");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Erreur d'imagerie");
        break;
      default:
        Serial.println("Erreur inconnue");
        break;
    }
  }

  p = finger.image2Tz(1);

  if (p != FINGERPRINT_OK)
  {
    Serial.println("Erreur de conversion de l'image. Veuillez réessayer.");
    return false;
  }

  /*global_var = "-100";
  Serial.println("Retirez le doigt");
  delay(10000);
  
  p = 0;

  while (p != FINGERPRINT_NOFINGER)
  {
    p = finger.getImage();
  }
*/
  p = finger.storeModel(id);

  if (p == FINGERPRINT_OK)
  {
    global_var = String(id);
    Serial.println("Empreinte enregistrée avec succès.Enlever votre doigt du capteur");
      p = 0;
      while (p != FINGERPRINT_NOFINGER)
      {
        p = finger.getImage();
      }

    return true;
  }
  else
  {
    Serial.println("Erreur lors de l'enregistrement de l'empreinte. Veuillez réessayer.");
    return false;
  }
}
//----------------------------------------------------Fonction de suppression d'une empreinte------------------------------------------------
void supprimerEmpreinte(uint8_t id) {
  Serial.println("Prêt à supprimer une empreinte !");
  Serial.print("Suppression de l'empreinte avec l'ID #");
  Serial.println(id);

  bool empreinteTrouvee = false;
  Data retrievedData;
  // Lire les données à partir de l'EEPROM
  EEPROM.get(0, retrievedData);

  // Recherche de l'empreinte dans le tableau des empreintes enregistrées
  for (uint8_t i = 0; i < retrievedData.nbEmpreintesEnregistrees; i++) {
    if (retrievedData.empreintesEnregistrees[i] == id) {
      empreinteTrouvee = true;
      break;
    }
  }

  if (empreinteTrouvee) {
    if (finger.deleteModel(id) == FINGERPRINT_OK) {
      Serial.println("Empreinte supprimée avec succès.");

      // Supprimer l'ID du tableau des empreintes enregistrées
      for (uint8_t i = 0; i < retrievedData.nbEmpreintesEnregistrees; i++) {
        if (retrievedData.empreintesEnregistrees[i] == id) {
          // Décaler les éléments suivants vers la gauche
          for (uint8_t j = i; j < retrievedData.nbEmpreintesEnregistrees - 1; j++) {
            retrievedData.empreintesEnregistrees[j] = retrievedData.empreintesEnregistrees[j + 1];
          }
          retrievedData.nbEmpreintesEnregistrees--;
          break;
        }
      }
      
      // Écrire les données mises à jour dans l'EEPROM
      EEPROM.put(0, retrievedData);
      EEPROM.commit();
      global_var = String(2000);
    } else {
      Serial.println("Impossible de supprimer l'empreinte. Veuillez réessayer.");
      
    }
  } else {
    Serial.println("ID d'empreinte non trouvé. Veuillez réessayer.");
    
  }
}

//-------------------------------------------------Fonction qui supprime tous les empreintes-------------------------------------------------
void supprimerToutesEmpreintes() {
  Serial.println("Suppression de toutes les empreintes enregistrées...");

  if (finger.emptyDatabase() == FINGERPRINT_OK) {
    Data data;
    Serial.println("Toutes les empreintes ont été supprimées avec succès.");
    // Réinitialise les empreintes enregistrées et le nombre d'empreintes
    for (int i = 0; i < data.nbEmpreintesEnregistrees; i++) {
      data.empreintesEnregistrees[i] = 0;
    }
    data.nbEmpreintesEnregistrees = 0;
    EEPROM.put(0, data); // Met à jour les données dans l'EEPROM
    EEPROM.commit(); // Enregistre les modifications dans l'EEPROM
    Serial.println(String(data.nbEmpreintesEnregistrees));
  } else {
    Serial.println("Erreur lors de la suppression des empreintes. Veuillez réessayer.");
  }
}



//-------------------------------------------------Fonction de communication avec l'application----------------------------------------------
void handlePostRequest(AsyncWebServerRequest *request) {

    
  if (request->hasParam("data", true)) {
    AsyncWebParameter* param = request->getParam("data", true);
    
    take_value = param->value();
    Serial.println(take_value);
    
    
    
    // Send response
    request->send(200, "text/plain", "Data received and processed");
  } else {
    
    request->send(300); // Bad request
  }
}

  
