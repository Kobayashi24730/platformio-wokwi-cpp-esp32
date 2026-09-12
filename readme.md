# platformio-wokwi-cpp-esp32

Simulação de um dispositivo IoT baseado em **ESP32** utilizando **PlatformIO** + **Wokwi**, com múltiplos sensores integrados e envio periódico de dados para uma API REST externa (dashboard hospedado em Render).

O firmware é escrito em **C++/Arduino** e faz leitura de movimento (PIR), temperatura/umidade (DHT22), distância (HC-SR04), som/analog (potenciômetro simulando sensor de som) e controla um LED indicador. Todas as leituras são enviadas via HTTPS (POST JSON) para o endpoint do dashboard.

---

## Sumário

- [Recursos](#recursos)
- [Arquitetura do Projeto](#arquitetura-do-projeto)
- [Hardware Simulado](#hardware-simulado)
- [Pinagem](#pinagem)
- [Pré-requisitos](#pré-requisitos)
- [Como executar](#como-executar)
- [Configuração](#configuração)
- [Formato do payload enviado à API](#formato-do-payload-enviado-à-api)
- [Estrutura de arquivos](#estrutura-de-arquivos)
- [Bibliotecas utilizadas](#bibliotecas-utilizadas)
- [Licença](#licença)

---

## Recursos

- Conexão automática ao Wi-Fi (`Wokwi-GUEST` por padrão para simulação).
- Leitura periódica de 5 sensores com intervalos independentes:
  - **PIR** — detecção de movimento (event-driven, dispara quando muda de estado).
  - **DHT22** — temperatura e umidade a cada 10 s.
  - **HC-SR04** — distância em cm a cada 5 s.
  - **Som/Analógico** — leitura ADC a cada 3 s.
  - **Métricas do sistema** — RSSI Wi-Fi, uptime e heap livre a cada 10 s.
- Envio HTTPS (`WiFiClientSecure` com `setInsecure()`) em JSON para a API do dashboard.
- LED indicador acionado ao detectar movimento.
- Feedback via Serial Monitor a 115200 baud.

---

## Arquitetura do Projeto

```
+----------------------+        Wi-Fi        +----------------------------------+
|   ESP32 (Wokwi)      | -----------------> |  API Dashboard (Render)          |
|                      |    HTTPS POST      |  /api/data/movimento             |
|  PIR / DHT22 /       |    JSON payload    |                                  |
|  HC-SR04 / ADC / LED |                    |                                  |
+----------------------+                    +----------------------------------+
```

---

## Hardware Simulado

Componentes definidos no `diagram.json` do Wokwi:

| Componente          | ID no diagrama | Função                              |
|---------------------|----------------|-------------------------------------|
| ESP32 DevKit V1     | `esp`          | Microcontrolador principal          |
| PIR Motion Sensor   | `pir1`         | Detecção de movimento               |
| DHT22               | `dht1`         | Temperatura e umidade               |
| HC-SR04             | `ultra1`       | Distância ultrassônica              |
| Potenciômetro       | `pot1`         | Simula sensor de som (entrada ADC)  |
| LED vermelho        | `led1`         | Indicador de movimento              |
| Resistor 220 Ω      | `r1`           | Limitador de corrente do LED        |

---

## Pinagem

| Sinal          | GPIO do ESP32 | Direção  |
|----------------|---------------|----------|
| PIR (OUT)      | `GPIO 4`      | Entrada  |
| DHT22 (DATA)   | `GPIO 14`     | Entrada  |
| HC-SR04 TRIG   | `GPIO 27`     | Saída    |
| HC-SR04 ECHO   | `GPIO 18`     | Entrada  |
| Som / ADC      | `GPIO 34`     | Entrada  |
| LED            | `GPIO 32`     | Saída    |

Alimentação: PIR, DHT22 e potenciômetro em 3.3 V; HC-SR04 em 5 V.

---

## Pré-requisitos

- [PlatformIO Core](https://platformio.org/install) ou a extensão do PlatformIO para VS Code.
- [Extensão Wokwi para VS Code](https://marketplace.visualstudio.com/items?itemName=Wokwi.wokwi-vscode) (para simular localmente a partir dos binários gerados).
- Git.

---

## Como executar

### 1. Clonar o repositório

```bash
git clone https://github.com/Kobayashi24730/platformio-wokwi-cpp-esp32.git
cd platformio-wokwi-cpp-esp32
```

### 2. Compilar o firmware

```bash
pio run
```

Isso gera `firmware.bin` e `firmware.elf` em `.pio/build/esp32dev/`, exatamente os caminhos referenciados pelo `wokwi.toml`.

### 3. Simular no Wokwi (VS Code)

Com a extensão do Wokwi instalada, abra o `diagram.json` e clique em **Start Simulation** — ou use o comando *Wokwi: Start Simulator* na paleta de comandos. A simulação carrega automaticamente o firmware compilado.

### 4. (Opcional) Gravar em hardware real

Conecte um ESP32 DevKit V1 via USB e execute:

```bash
pio run --target upload
pio device monitor
```

---

## Configuração

As principais constantes ficam no topo de `src/main.cpp`:

```cpp
const char* ssid     = "Wokwi-GUEST";
const char* password = "";
const char* api_url  = "seu endpoint da dashboard";
```

Para uso em hardware real, substitua `ssid` e `password` pela sua rede Wi-Fi e ajuste `api_url` se necessário. Se for consumir o endpoint em produção, considere validar o certificado TLS em vez de usar `client.setInsecure()`.

---

## Formato do payload enviado à API

Cada leitura é enviada como `POST` JSON com o seguinte formato:

```json
{
  "device_id": "ESP32_PIR_01",
  "sensor": "PIR",
  "estado": true,
  "value": 1.00,
  "timestamp": "2026-09-11T20:00:00.000Z"
}
```

`device_id` e `sensor` variam conforme a origem da leitura:

| `device_id`             | `sensor`     | `value`                 |
|-------------------------|--------------|-------------------------|
| `ESP32_WiFi_01`         | `WiFi`       | RSSI em dBm             |
| `ESP32_UPTIME_01`       | `UPTIME`     | Segundos desde o boot   |
| `ESP32_memori_01`       | `memori`     | Heap livre (bytes)      |
| `ESP32_PIR_01`          | `PIR`        | 1.0 / 0.0               |
| `ESP32_SOUND_01`        | `SOUND`      | Valor bruto do ADC      |
| `ESP32_ULTRASONIC_01`   | `ULTRASONIC` | Distância em cm         |
| `ESP32_TEMP_01`         | `DHT22`      | Temperatura em °C       |
| `ESP32_HUMID_01`        | `HUMIDITY`   | Umidade relativa (%)    |

> Observação: o `timestamp` atualmente é enviado como valor fixo no código. Se for necessário timestamp real, integre NTP via `configTime()`/`time.h`.

---

## Estrutura de arquivos

```
.
├── .vscode/            # Configurações do VS Code
├── src/
│   └── main.cpp        # Firmware principal
├── diagram.json        # Circuito do Wokwi
├── platformio.ini      # Configuração do PlatformIO
├── wokwi.toml          # Configuração da simulação Wokwi
└── README.md
```

---

## Bibliotecas utilizadas

Declaradas em `platformio.ini`:

- [`adafruit/DHT sensor library`](https://github.com/adafruit/DHT-sensor-library) `^1.4.6`
- [`adafruit/Adafruit Unified Sensor`](https://github.com/adafruit/Adafruit_Sensor) `^1.1.14`

Além das bibliotecas nativas do core Arduino-ESP32: `WiFi.h`, `HTTPClient.h`, `WiFiClientSecure.h`.

---

## Licença
Este projeto está licenciado sob a licença MIT — veja o arquivo LICENSE para detalhes.