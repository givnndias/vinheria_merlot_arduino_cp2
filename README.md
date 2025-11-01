# 🍷 Vinheria Merlot – Sistema de Monitoramento Ambiental (Arduino CP2)

## 📘 Descrição

Projeto desenvolvido para monitorar variáveis ambientais em ambientes de armazenamento de vinhos, utilizando **Arduino Uno** e simulação no **Wokwi**. O sistema controla **temperatura**, **luminosidade** e **umidade**, garantindo condições ideais para preservar a qualidade dos produtos da Vinheria Merlot.


## 🎯 Objetivo do Projeto  
O projeto **Vinheria Merlot** tem como objetivo desenvolver um **sistema de monitoramento ambiental automatizado** para auxiliar na conservação adequada de vinhos em adegas.  
Utilizando um **Arduino Uno**, sensores de **temperatura**, **umidade** e **luminosidade**, o sistema monitora constantemente o ambiente, exibindo os valores em um **display LCD** e acionando alertas visuais e sonoros caso as condições ideais sejam ultrapassadas.

---

## ⚙️ Componentes Utilizados

| Componente | Quantidade | Função Principal |
|-------------|-------------|------------------|
| Arduino Uno | 1 | Microcontrolador principal |
| Sensor DHT22 | 1 | Leitura de temperatura e umidade |
| Sensor LDR | 1 | Detecção de luminosidade ambiente |
| Display LCD 16x2 (I2C) | 1 | Exibição das medições em tempo real |
| Teclado Matricial (Keypad 4x4) | 1 | Interface para inserção de comandos ou configurações |
| LEDs (Vermelho, Amarelo, Verde) | 3 | Indicação visual do estado ambiental |
| Buzzer | 1 | Alerta sonoro em caso de condição crítica |
| Resistores (220Ω e 10kΩ) | 4+ | Limitação de corrente e divisor de tensão para o LDR |
| Protoboard + Jumpers | 1 conjunto | Interligação dos componentes |

---

## 🧩 Diagrama de Montagem

![Diagrama de Montagem](https://raw.githubusercontent.com/givnndias/vinheria_merlot_arduino_cp2/main/vinheria_merlot_wokwi.png)

📷 *Figura 1 – Diagrama de montagem do circuito no Wokwi (Merlot, 2025)*

---

## 💡 Funcionamento do Sistema

O sistema realiza **leituras contínuas** das variáveis ambientais e atua conforme as condições detectadas:

1. O **DHT22** mede temperatura e umidade.  
2. O **LDR** detecta o nível de luminosidade do ambiente.  
3. Os valores são exibidos em tempo real no **LCD 16x2**.  
4. Caso algum valor ultrapasse os limites configurados (faixas ideais), são acionados os seguintes alertas:
   - 🔴 **LED Vermelho + Buzzer:** condição crítica (temperatura/umidade fora do ideal).  
   - 🟡 **LED Amarelo:** atenção (valores próximos ao limite).  
   - 🟢 **LED Verde:** condições ideais.  
5. O **keypad** permite ao usuário configurar parâmetros ou navegar por menus de controle.

---

## 📊 Faixas de Controle (Valores de Referência)

| Parâmetro | Faixa Ideal | Atenção | Crítico |
|------------|-------------|---------|----------|
| Temperatura | 10 °C – 15 °C | 8 °C – 17 °C | < 8 °C ou > 17 °C |
| Umidade | 60 % – 70 % | 50 % – 80 % | < 50 % ou > 80 % |
| Luminosidade | Baixa | Média | Alta |

---

## 👥 Equipe

- Giovanna Dias - 566647
- Maria Laura Druzeic - 566634
- Marianne Mukai - 568001

---

## 🧠 Conclusão  
O projeto **Vinheria Merlot** demonstra a aplicação prática de sistemas embarcados e Internet das Coisas (IoT) no controle ambiental de espaços sensíveis, como adegas. Ele alia sensores, interface física e alertas automatizados, promovendo um monitoramento eficiente, acessível e didático.







