Wrote a DHT11 driver for STM32F303K8.

To use, use the DHT config struct (DHT_Config_t) to select the GPIO port (A, B, or F) as well as the pin number connected to the data line of the DHT11.
Then, select one of the available timers (3, 7, 15, 16, 17) to be used to collect the temperature/humidity data as well as the data read timeout (Recommended 100,000).
Pass the DHT config to `DHT_Init()` to initialize all the GPIO and timer peripherals before calling `GetDHTData` with a size 5 uint8 array.

Example: <br>
<img width="402" height="186" alt="image" src="https://github.com/user-attachments/assets/9ee8cd5f-f37f-4467-a5fa-da56a61a6d66" />
<br>
<img width="1149" height="443" alt="image" src="https://github.com/user-attachments/assets/69d151a3-2fc1-4e9c-b625-c99fc8d65f51" />
<br>
