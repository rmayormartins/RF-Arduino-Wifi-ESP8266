/*

//no diretorio: python -m SimpleHTTPServer
//no servidor so precisa: index.htm

Arduino Due + ESP 8266 WiFi Module
- As STA to Join AP
- Connect to website as client

Serial (Tx/Rx) communicate to PC via USB
Serial3 (Tx3/Rx3) connect to ESP8266
Tx3 - ESP8266 Rx
Rx3 - ESP8266 Tx
ESP8266 CH_PD Connect to ESP8266 VCC

for firmware:
"v0.9.5.2 AT Firmware"
(http://goo.gl/oRdG3s)
AT version:0.21.0.0
SDK version:0.9.5

*/
#define ESP8266 Serial3    //usar a serial 3 (mas vou renomear ela como ESP8266, porque o modulo vai estar ligado nessa serial (RX3,TX3) do Mega ou do DUE)
String SSID = "julinho"; //tenho um formato de dados string (cadeia de caracteres), estou dizendo que SSID eh julinho
String PASSWORD = "123456789"; //mesma coisa aqui

int LED = 13; //dizendo que quando o programa ler a palavra LED, ele vai entender que eh o valor 13.

boolean FAIL_8266 = false; //eh um tipo de dado booleano, ou seja, eh V ou F

#define BUFFER_SIZE 1024
char buffer[BUFFER_SIZE]; //reserva 1024 bytes da memoria para escrever algo

void setup() {
  pinMode(LED, OUTPUT);   //Estou atrelando esse LED (que eh o pino 13) como uma saida
 
  digitalWrite(LED, LOW); //escreve no pino 13, um valor baixo (que eh 0)
  delay(300);
  digitalWrite(LED, HIGH); //escreve no pino 13, um valor alto (que eh 1) -pra piscar e mostrar funcionamento
  delay(200);
  digitalWrite(LED, LOW);
  delay(300);
  digitalWrite(LED, HIGH);
  delay(200);
  digitalWrite(LED, LOW);

  do{
    //Serial.begin(9600);
    //ESP8266.begin(9600);
    Serial.begin(19200); //inicia a serial (TX0, RX0)
    ESP8266.begin(19200); //inicia a Serial3 (TX3,RX3) - depois vc pode mudar para os pinos 19,18 de acordo com o Fernando
 
    //Wait Serial Monitor to start
    while(!Serial);
    Serial.println("--- Start ---");

    ESP8266.println("AT+RST"); //De acordo com https://www.espressif.com/sites/default/files/documentation/4a-esp8266_at_instruction_set_en.pdf (Instrucoes de comando AT para o modulo ESP). O comando AT+RST: ele vai REINICIAR o modulo
    delay(1000);
    if(ESP8266.find("ready")) //a resposta do modulo vai ser estar pronto. o ESP8266.find vai procurar no monitor serial se encontrou a palavra ready (pronto), se encontrou, ele vai imprimir no monitor serial q esta de fato pronto.
    {
      Serial.println("Module is ready");
     
      ESP8266.println("AT+GMR"); //Vai checar a versao do modulo
      delay(1000);
      clearESP8266SerialBuffer(); //limpa o buffer do modulo.
     
      ESP8266.println("AT+CWMODE=1"); //Vai configurar o modulo como cliente se deixar =1, como access point se deixar =2, como os dois se deixar =3;
      //ESP8266.println("AT+CWMODE=3");
      delay(2000);
     
      //Quit existing AP, for demo
      Serial.println("Quit AP"); //Ate esse momento aqui era teste no modulo, e vai sair de qualquer lugar que estiver conectado.
      ESP8266.println("AT+CWQAP"); //Desconecta de qualquer AP
      delay(1000);
      clearESP8266SerialBuffer();
     
      if(cwJoinAP())   //Essa funcao foi construida la embaixo. SE ele se conectou ao AP com sucesso, ele vai executar esse escopo. 
      {
        Serial.println("CWJAP Success"); 
        FAIL_8266 = false;
       
        delay(3000);
        clearESP8266SerialBuffer();
        //Get and display my IP
        sendESP8266Cmdln("AT+CIFSR", 1000); //Pegar o endereço IP local
        //Set multi connections
        sendESP8266Cmdln("AT+CIPMUX=1", 1000); //Esse comando eh para permitir multiplas conexoes TCP.
        //sendESP8266Cmdln("AT+CIPMUX=0", 1000); //=0 significa que eh SINGLE (uma conexao), =1 significa que eh MULTIPLAS (conexoes)

        Serial.println("Setup finish");
      }else{ //SENAO conectou com sucesso, ele vai dizer no monitor serial que a conexao falhou
        Serial.println("CWJAP Fail");
        delay(500);
        FAIL_8266 = true;
      }
    }else{ //La emcima estava o IF que dizia SE o modulo estiver pronto, vai executar todo escopo. SENAO ele vai dizer que nao houve resposta.
      Serial.println("Module have no response.");
      delay(500);
      FAIL_8266 = true;
    }
  }while(FAIL_8266);
 
  digitalWrite(LED, HIGH);
 
  //set timeout duration ESP8266.readBytesUntil
  ESP8266.setTimeout(1000);
}

//Tudo anteriormente a isso, era configuracao do modulo e conexao ao AP.


void loop(){ //Aqui eh o q o modulo vai ficar executando.
  /*
  AT+CIPSTART=id,"type","addr",port
  id = 0
  type = "TCP"
  addr = "www.example.com"
  port = 80
  */
  String TARGET_ID="0";
  String TARGET_TYPE="TCP";
  String TARGET_ADDR="192.168.1.201"; //esse eh o IP do AP (do roteador)
  String TARGET_PORT="8000";
  //String TARGET_ADDR="www.google.com";
  //String TARGET_PORT="80";

  String cmd="AT+CIPSTART=" + TARGET_ID; //Esse comando AT vai estabelecer uma conexao TCP com o AP (no endereço ="TCP","iot.espressif.cn",8000 ) 
  cmd += ",\"" + TARGET_TYPE + "\",\"" + TARGET_ADDR + "\"";
  cmd += ","+ TARGET_PORT;

  Serial.println(cmd);
  ESP8266.println(cmd); //Vai mandar pro modulo esse comando cmd, com a sequencia ali de cima no AT+CIPSTART...
  delay(1000);
  //Assume OK
  //display and clear buffer
  clearESP8266SerialBuffer();
 
  /*
  GET / HTTP/1.1\r\n
  Host: www.example.com:80\r\n\r\n
  */
  String HTTP_RQS = "GET / HTTP/1.1\r\n"; //Esse GET vai enviar uma REQUISICAO pro servidor (Por exemplo, a UID capturada pelo RFID, ele pode ir no lugar do "1.1", dessa forma vai aparecer no terminal do Servidor)
  HTTP_RQS += "Host: " + TARGET_ADDR; //Esse GET tem esse formato (endereço+porta)
  HTTP_RQS += ":" + TARGET_PORT + "\r\n\r\n";
 
  String cmdSEND_length = "AT+CIPSEND="; //Esse comando AT vai enviar dados.
  cmdSEND_length += TARGET_ID + "," + HTTP_RQS.length() +"\r\n"; //Como que ele vai enviar esses dados: ele vai estabelecer um tamanho de mensagem para ser enviada.
 
  ESP8266.print(cmdSEND_length);
  Serial.println(cmdSEND_length);
 
  Serial.println("waiting >");
 
  if(!ESP8266.available());
 
  if(ESP8266.find(">")){
    Serial.println("> received"); //Testa se houve sucesso na requisicao do servidor
    ESP8266.println(HTTP_RQS);
    Serial.println(HTTP_RQS);
   
    boolean OK_FOUND = false;
   
    //program blocked untill "SEND OK" return
    while(!OK_FOUND){
      if(ESP8266.readBytesUntil('\n', buffer, BUFFER_SIZE)>0){
        Serial.println("...");
        Serial.println(buffer);
       
        if(strncmp(buffer, "SEND OK", 7)==0){  //Se houve uma transmissao correta, vai aparecer SEND OK
          OK_FOUND = true;
          Serial.println("SEND OK found");
        }else{
          Serial.println("Not SEND OK...");
        }
      }
    }
//
    if (Serial3.find("apelha")) //Se esta palavra estiver em index.htm que esta dentro do diretorio que virou o SERVIDOR, vai aparecer no monitor serial a msg SIM
{
//If the string was found we know the page is up and we turn on the LED status
//light to show the server is ONLINE
Serial.println("sim");
}
else //Se aquela palavra buscada com .find nao estiver em index.htm (e nao apareceu no monitor serial), vai aparecer a msg NAO
{
//If the string was not found then we can assume the server is offline therefore
//we should turn of the light.
Serial.println("não");
}

    if(OK_FOUND){  //Esse escopo ele vai procurar por codigos html para apresentar no monitor serial.
     
      //Dummy display received data
      //until connection CLOSED, "</HTML>" or "</html>" received
      //only compare beginning of lines
      int i;   
      while((i=ESP8266.readBytesUntil('\n', buffer, BUFFER_SIZE-1))>=0){
        buffer[i] = '\0';  //insert terminator
        Serial.println(buffer);
       
        //have to match with TARGET_ID
        if(strncmp(buffer, "0,CLOSED", 7)==0){
          Serial.println("CLOSED");
          break;
        }
        if(strncmp(buffer, "</HTML>", 7)==0){
          Serial.println("</HTML> found");
          break;
        }
        if(strncmp(buffer, "</html>", 7)==0){
          Serial.println("</html> found");
          break;
        }
      }
    }
   
  }else{
    Serial.println("> NOT received, something wrong!");
  }
 
  //Close connection
  String cmdCIPCLOSE = "AT+CIPCLOSE=" + TARGET_ID; //Encerra a conexao
  ESP8266.println(cmdCIPCLOSE);
  Serial.println(cmdCIPCLOSE);
 
  delay(10000);
 
}

boolean waitOKfromESP8266(int timeout) //Esse bloco eh uma funcao para avaliar o tempo de resposta do modulo.
{
  do{
    Serial.println("wait OK...");
    delay(1000);
    if(ESP8266.find("OK"))
    {
      return true;
    }

  }while((timeout--)>0);
  return false;
}

//Essa funcao cwJoinAP vai utilizar o comando AT para se conectar ao AP. 
boolean cwJoinAP()  //Eh booleano pq vai retornar uma resposta V ou F
{
  String cmd="AT+CWJAP=\"" + SSID + "\",\"" + PASSWORD + "\""; //O comando AT+CWJAP vai conectar a um AP. Mas qual ap? o AP que tiver a SSID julinho e o password 123456789
  ESP8266.println(cmd); //Ele vai enviar pela Serial3, o comando AT para o modulo.
  return waitOKfromESP8266(10);
}

//Send command to ESP8266, assume OK, no error check
//wait some time and display respond
void sendESP8266Cmdln(String cmd, int waitTime) //Essa funcao espera um determinado tempo pra efetuar alguma acao 
{
  ESP8266.println(cmd);
  delay(waitTime);
  clearESP8266SerialBuffer();
}

//Basically same as sendESP8266Cmdln()
//But call ESP8266.print() instead of call ESP8266.println()
void sendESP8266Data(String data, int waitTime)
{
  //ESP8266.print(data);
  ESP8266.print(data);
  delay(waitTime);
  clearESP8266SerialBuffer();
}

//Clear and display Serial Buffer for ESP8266
void clearESP8266SerialBuffer()
{
  Serial.println("= clearESP8266SerialBuffer() =");
  while (ESP8266.available() > 0) {
    char a = ESP8266.read();
    Serial.write(a);
  }
  Serial.println("==============================");
}
