# Introdução
Código para Arduino Nano (Atmega328p) para controlar o pulso da solda ponto caseira, inspirado no [projeto do Marlon Nardi](https://marlonnardi.com/2023/12/03/como-fazer-solda-ponto-profissional-para-baterias-18650-e-mais-construa-sua-propria-bicicleta-eletrica-1/). Apesar do princípio ser o mesmo, o código é radicalmente diferente. A versão mínima não usa o alto-falante e o botão no eixo do encoder.

Importante: não foi testado utilizando microinterruptor sem debounce, ou seja, apenas o switch ligado diretamente no pino de controle com pullup do hardware da microcontroladora.

A documentação é parcial e pode ser entendida de forma completa primeiro estudando o [projeto original](https://marlonnardi.com/2023/12/03/como-fazer-solda-ponto-profissional-para-baterias-18650-e-mais-construa-sua-propria-bicicleta-eletrica-1/) no qual foi baseado.

# Material necessário
1. Arduino Nano (Atmega328p)
2. Display OLED 1.3" I2C 12864 (SH1106G)
3. Fim de curso mecânico RAMPS 1.4
4. Relé de estádo sólido (SSR) 40A DA (DC/AC)
5. Encoder / Decoder rotativo KY-040
6. Alto-falante / buzzer piezo passivo de 12mm e 3.3-5v (opcional)¹

¹ Ao utilizar o buzzer ligado diretamente nos pinos da microcontroladora, certifique-se que a corrente não exceda 40mA, ou pode ocorrer danos ao Arduino. Na dúvida, utilize um buzzer ligado a um circuito com resistor limitador de corrente e transistor como driver (ex.: 8550/9012).

# Ligação

![SCH](images/SCH.PNG)

# Exemplos 
Imagens da captura do osciloscópio, pulsos configurados em 25, 70 e 10 millisegundos, respectivamente:

![25ms](images/IMAGE1.PNG)

![70ms](images/IMAGE2.PNG)

![10ms](images/IMAGE3.PNG)

Por motivos que eu desconheço, há um desvio de +0.004ms a cada 1ms adicionado na duração do pulso. Exemplificando, configurado um pulso de 10ms, o osciloscópio mede um sinal com 10.04ms de duração, enquando um pulso de 50ms dura 50.20ms, 100ms dura 100.40ms e etc.
