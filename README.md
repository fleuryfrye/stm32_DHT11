Wrote a DHT11 driver for STM32F3 series.

To use, use the DHT config struct (DHT_Config_t) to select the GPIO port (A, B, or F) as well as the pin number connected to the data line of the DHT11.
Then, select one of the available timers (3, 7, 15, 16, 17) to be used to collect the temperature/humidity data as well as the data read timeout (Recommended 100,000).
Pass the DHT config to `DHT_Init()` to initialize all the GPIO and timer peripherals before calling `GetDHTData` with a size 5 uint8 array.

Example: <br>
<img width="402" height="186" alt="image" src="https://github.com/user-attachments/assets/9ee8cd5f-f37f-4467-a5fa-da56a61a6d66" />
<br>
<img width="1149" height="443" alt="image" src="https://github.com/user-attachments/assets/69d151a3-2fc1-4e9c-b625-c99fc8d65f51" />
<br>


Note: after writing the core logic to set up the peripherals and reading data from the DHT11 sensor, I used AI to transform the logic to let the driver be configurable
to any of the GPIO ports/pins and timers available on the F3K08 and the boiler plate to make the driver more readable. This was part of my experiment of using AI in my
workflow. This did require a lot of guidance and code review to get into decent shape, and I'm unsure if it really saved me much time in the end, but I think it is 
good to be familiar with the tools available to see where it works best and falls short.
