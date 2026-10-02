# Patinete IoT — ESP32 + Wokwi

Simulação de patinete elétrico compartilhado com desbloqueio por QR Code, geofence por GPS simulado, alarme antifurto e detecção de estação correta/incorreta. Feito para rodar no [Wokwi](https://wokwi.com/projects/447013218328670209) com ESP32 DevKit C V4 + OLED SSD1306.

## Como funciona

1. **Bloqueado:** OLED exibe QR Code. Patinete aguardando leitura.
2. **Desbloqueio:** aperte o botão verde (`btn1`, pino 17).
   - `sw1` (pino 23) em posição 1 = leitura válida → LED verde + buzzer 2x, destrava, salva referência do ultrassônico e mostra LAT/LNG.
   - `sw1` em posição 3 = leitura inválida → LED vermelho + buzzer grave 1s, mantém QR.
3. **Em viagem:** a cada 5s lê o HC-SR04 e converte deslocamento em coordenadas a partir de `-3.742221, -38.535461`. OLED e Serial mostram posição.
4. **Geofence 200 m:** se passar de 200 m da origem, entra em `ALERTA! Fora da area segura!` com buzzer/LED alternando. Voltando para dentro do raio por 5 s, desativa e retoma coordenadas.
5. **Estacionar:** acione o PIR (`pir2`, pino 22, Sensor de estacionamento).
   - `sw2` (pino 13) em estação válida → buzzer curto, entra em `Patinete carregando...`, encerra a viagem (reinicie a simulação).
   - `sw2` em estação inválida → `Estação incorreta!` por 3 s com LED vermelho + buzzer, depois volta às coordenadas.

## Hardware (Wokwi `diagram.json`)

| Componente | Modelo Wokwi | Pino ESP32 |
|---|---|---|
| OLED | `board-ssd1306` 0x3c | SDA 2, SCL 15 |
| GPS simulado | `wokwi-hc-sr04` | TRIG 19, ECHO 18 |
| Sensor estacionamento | `wokwi-pir-motion-sensor` | OUT 22 |
| Botão autenticação (QR) | `wokwi-pushbutton-6mm` | 17 (`INPUT_PULLUP`) |
| Switch leitura QR | `wokwi-slide-switch` sw1 | 23 |
| Switch estação | `wokwi-slide-switch` sw2 | 13 |
| LED RGB (só R+G) | `wokwi-rgb-led` | R 4, G 0 |
| Buzzer | `wokwi-buzzer` | 16 |
| Protoboard + resistores 200 Ω | — | — |

Bibliotecas (`libraries.txt`): `SSD1306`, `QRcodeOled`.

## GPS simulado

- `1 cm` no ultrassônico = `2 m` de deslocamento (`deslocamentoMetros = abs(variaçãoCM) * 2.0`).
- Direção lat/lng sorteada a cada desbloqueio (`random()`), travada em `-4.0..-3.5 / -38.6..-38.4`.
- Atualização a cada 5 s (`intervaloGPS`). Leitura inválida é ignorada no Serial.

## Simular no Wokwi

1. Abra o projeto no Wokwi e dê play.
2. Deixe `Interruptor de QR Code` em 1 (válida) e aperte o botão verde para destravar.
3. Mova o slider do `Módulo GPS` (HC-SR04) para deslocar o patinete; PIR para estacionar.
4. Para testar erro: QR em 3 e aperte o botão; estação em 3 e acione o PIR; ou afaste >200 m para ouvir o alarme.
5. Monitor Serial em 115200 mostra `[GPS]`, `[ESTAÇÃO]`, `[ALERTA]`.

## Arquivos

- `sketch.ino` — firmware completo (setup/loop, OLED, QR, ultrassônico, PIR, LEDs, buzzer, geofence).
- `diagram.json` — circuito e instruções da simulação no Wokwi.
- `libraries.txt` — dependências Wokwi.
- `wokwi-project.txt` — link original do projeto.
