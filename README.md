# 📊 Data Logger Ambiental com Arduino

Este projeto consiste em um dispositivo de registro de dados (data logger) dedicado ao monitoramento de condições ambientais em espaços controlados. O equipamento utiliza um microcontrolador ATMEGA328P para monitorar e registrar dados de **temperatura**, **umidade relativa do ar** e **luminosidade**, com alertas sonoros e visuais caso os parâmetros saiam das faixas pré-definidas.

-----

## 📋 Índice

  * Funcionalidades
  * Componentes Necessários
  * Diagrama de Conexões
  * Como Funciona
  * Bibliotecas
  * Instalação e Uso
  * Colaboradores

-----

## ✨ Funcionalidades

  * **Monitoramento em Tempo Real**: Exibe os dados atuais de temperatura, umidade e luminosidade em um display LCD 16x2.
  * **Registro de Dados (Data Logging)**: Armazena leituras na memória EEPROM interna quando os valores ultrapassam os limites configurados. Cada registro inclui um timestamp preciso.
  * **Alertas Visuais e Sonoros**: Um LED vermelho e um buzzer são ativados quando qualquer medição está fora da faixa operacional segura. Um LED verde indica status normal.
  * **Interface de Usuário Intuitiva**: Um menu navegável permite visualizar dados, consultar o log e ajustar parâmetros do sistema.
  * **Timestamp Preciso**: Utiliza um módulo RTC (Real-Time Clock) DS1307 para garantir que cada registro de dados tenha data e hora exatas.
  * **Limites Configuráveis**: O limite máximo de luminosidade para o acionamento de alertas pode ser ajustado diretamente pelo menu do dispositivo.

-----

## ⚙️ Componentes Necessários

| Componente | Descrição |
| :--- | :--- |
| Microcontrolador | Placa baseada no **ATMEGA328P** (Ex: Arduino Uno, Nano) |
| Display | **LCD 16x2 com módulo I2C** |
| Sensor de Temp/Umid. | **DHT11** |
| Sensor de Luminosidade | **LDR (Light Dependent Resistor)** + resistor de 10kΩ |
| Relógio | Módulo **RTC DS1307** |
| Armazenamento | **Memória EEPROM** interna do ATMEGA328P |
| Atuadores de Alerta | 1x **Buzzer passivo**, 1x **LED Verde**, 1x **LED Vermelho** |
| Controles | 1x **Joystick analógico** (ou botões direcionais), 2x **Push Buttons** |
| Outros | Protoboard, Jumpers, Resistores para os LEDs |

-----

## 🔌 Diagrama de Conexões

| Componente | Pino no Arduino |
| :--- | :--- |
| Display LCD (SDA) | `A4` |
| Display LCD (SCL) | `A5` |
| Módulo RTC (SDA) | `A4` |
| Módulo RTC (SCL) | `A5` |
| Sensor DHT11 | `D2` |
| Botão "Entra" | `D3` |
| Botão "Volta" | `D4` |
| LED OK (Verde) | `D5` |
| LED Falha (Vermelho) | `D6` |
| Buzzer | `D8` |
| Joystick (Eixo X) | `A0` |
| Sensor LDR | `A2` |

<img width="983" height="868" alt="image" src="https://github.com/user-attachments/assets/f29fc13e-2b62-43e9-8f06-a86e41a147e8" />


-----

## 🚀 Como Funciona

### Monitoramento e Alertas

O `loop()` principal verifica os sensores a cada minuto. Os valores de temperatura, umidade e luminosidade são comparados com as faixas de operação seguras (triggers):

  * **Faixa de Temperatura**: 15 °C \< T \< 25 °C
  * **Faixa de Umidade**: 30% \< U \< 50%
  * **Faixa de Luminosidade**: 0 \< L \< 30% (valor máximo ajustável)

Se qualquer medição estiver fora desses limites, a função `handleAlarm()` é chamada, ativando o **LED vermelho** e o **buzzer**. Caso contrário, o **LED verde** permanece aceso.

### Registro de Dados na EEPROM

Quando uma medição fora da faixa é detectada, o sistema grava um novo registro na EEPROM. Cada registro ocupa **10 bytes** e possui a seguinte estrutura:

  * **Timestamp** (4 bytes): Tempo Unix (segundos desde 01/01/1970).
  * **Temperatura** (2 bytes): Valor multiplicado por 100 e armazenado como inteiro.
  * **Umidade** (2 bytes): Valor multiplicado por 100 e armazenado como inteiro.
  * **Luminosidade** (2 bytes): Valor analógico bruto (0-1023).

O sistema pode armazenar até **120 registros**. Ao atingir o limite, ele sobrescreve os registros mais antigos (comportamento de buffer circular).

### Interface e Navegação

O usuário navega pelo menu usando um joystick e dois botões (`Entra` e `Volta`):

  * **1. Data e Hora**: Exibe a data e hora atuais obtidas do módulo RTC.
  * **2. Sensores**:
      * **2.1 Hum e Temp**: Mostra as leituras atuais dos sensores no display.
      * **2.2 Ajuste Luz**: Permite aumentar ou diminuir o valor máximo do trigger de luminosidade.
  * **3. Ver Log**: Envia todos os registros armazenados na EEPROM para o **Monitor Serial** do Arduino IDE, formatados de maneira legível.

### Video demonstrativo
 https://youtube.com/shorts/Jy3Repk7K6I?si=oD8lZE54iMaD-ZcR

-----

## 📚 Bibliotecas

Para compilar o código, você precisará instalar as seguintes bibliotecas através do Gerenciador de Bibliotecas da Arduino IDE:

  * `LiquidCrystal_I2C` by Frank de Brabander
  * `RTClib` by Adafruit
  * `DHT sensor library` by Adafruit
  * `CMBMenu` (Esta biblioteca precisa ser adicionada manualmente ao projeto, pois não está no gerenciador padrão).

-----

## 💻 Instalação e Uso

1.  **Montagem**: Monte o hardware seguindo o [Diagrama de Conexões](https://www.google.com/search?q=%23-diagrama-de-conex%C3%B5es).
2.  **Bibliotecas**: Instale todas as bibliotecas listadas na seção anterior.
3.  **Configuração Inicial do RTC**:
      * Descomente a linha `//RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));` na função `setup()`.
      * Faça o upload do código para o Arduino. Isso ajustará o relógio para a hora do seu computador.
      * **Comente a linha novamente** e faça o upload uma segunda vez. Isso evita que o relógio seja resetado toda vez que o dispositivo ligar.
4.  **Uso**:
      * Navegue entre as opções do menu usando o joystick (ou botões) para a esquerda e direita.
      * Pressione o botão **"Entra"** para acessar um submenu ou executar uma ação.
      * Pressione o botão **"Volta"** para retornar ao menu anterior.
      * Para visualizar os logs salvos, selecione a opção "3. Ver Log" e abra o Monitor Serial na Arduino IDE com a velocidade de **9600 baud**.

-----

## 🤝 Colaboradores

Agradecemos às seguintes pessoas que contribuíram para este projeto:

  * **Diogo Santos Rodrigues** - [@dioguit0s](https://github.com/dioguit0s/)
  * **Gustavo Sgrignoli Marmo** - [@gustavomarmo](https://github.com/gustavomarmo)
  * **Leonardo Rosario Teixeira** - [@leonardorosario](https://github.com/leonardorosario)
  * **Ryan Corazza Alvarenga** - [@aishiterai](https://github.com/aishiteirai)
  * **Bianca Ricci Lima** - [@biaricci](https://github.com/biaricci)
