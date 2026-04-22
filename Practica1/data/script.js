// Configuración del WebSocket

// window.location.hostname es la IP del ESP32 (ej: 192.168.1.100)
let gateway = `ws://${window.location.hostname}/ws`;
let ws;

/*
  Envio de mensajes para que el ESP32 controle los dispositivos
*/
const deviceStates = {
  luz1: false,
};

// ── Conectar WebSocket ──
function conectarWS() {
  ws = new WebSocket(gateway);

  ws.onopen = () => {
    addMessageToLog("WebSocket conectado al ESP32");
  };

  ws.onclose = () => {
    addMessageToLog("Conexión cerrada. Reintentando en 3s…", "err");
    // Reconexión automática
    setTimeout(conectarWS, 3000);
  };

  ws.onerror = (e) => {
    addMessageToLog("Error de WebSocket", "err");
  };

  ws.onmessage = (event) => {
    // El ESP32 siempre manda JSON: {"temp":24.5,"hum":60.0,"led":true}
    try {
      const datos = JSON.parse(event.data);
      console.log(
        `temp=${datos.temp}°C  hum=${datos.hum}%  led=${datos.led}`,
        "recv",
      );
      // Llamamos a tu función con los datos nuevos
      updateTempyHum(datos.temp, datos.hum);
    } catch (e) {
      console.log("JSON inválido: " + event.data, "err");
    }
  };
}

// Variables globales
let temperatureData = [];
let temperatureChart;

// Elementos DOM
const temperatureEl = document.getElementById("temperature");
const humidityEl = document.getElementById("humidity");
const messageLog = document.getElementById("message-log");

// Inicializar gráfico
function initChart() {
  const ctx = document.getElementById("temperature-chart").getContext("2d");

  temperatureChart = new Chart(ctx, {
    type: "line",
    data: {
      labels: [],
      datasets: [
        {
          label: "Temperatura °C",
          data: temperatureData,
          borderColor: "#2563eb",
          tension: 0.1,
          fill: false,
        },
      ],
    },

    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        y: {
          beginAtZero: false,
        },
      },
    },
  });
}

//Actualizar datos de temperatura y humedad
function updateTempyHum(newTemp, newHumidity) {
  temperatureEl.textContent = newTemp.toFixed(1);
  humidityEl.textContent = newHumidity.toFixed(1);

  // Actualizar gráfico
  temperatureData.push(newTemp);
  if (temperatureData.length > 15) temperatureData.shift();

  temperatureChart.data.labels = Array.from(
    { length: temperatureData.length },
    (_, i) => i + 1,
  );
  temperatureChart.data.datasets[0].data = temperatureData;
  temperatureChart.update();
  addMessageToLog(`Actualización: T=${newTemp}°C H=${newHumidity}%`);
}

// Añadir mensaje al log
function addMessageToLog(message) {
  const now = new Date(); //Esto se deberuia cambiar por la hora del RTC del esp32
  const timeString = now.toLocaleTimeString();
  const messageElement = document.createElement("p");
  messageElement.textContent = `[${timeString}] ${message}`;
  messageLog.appendChild(messageElement);
  messageLog.scrollTop = messageLog.scrollHeight;
}

function cambioEstado(event, topic) {
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    addMessageToLog("Sin conexión, no se puede enviar", "err");
    return;
  }
  const boton = event.currentTarget;
  // Cambiar el estado global
  const state = deviceStates[topic];
  if (state) {
    boton.classList.remove("btn-danger");
    boton.classList.add("btn-success");
    boton.textContent =
      boton.textContent.split(" ").slice(0, -1).join(" ") + " encendida";
    //addMessageToLog(`Led encendido`);
  } else {
    boton.classList.remove("btn-success");
    boton.classList.add("btn-danger");
    boton.textContent =
      boton.textContent.split(" ").slice(0, -1).join(" ") + " apagada";
    //addMessageToLog(`Led apagado`);
  }
  toggleLed(state);
  deviceStates[topic] = !deviceStates[topic];
}

// ── Enviar comando al ESP32 para cambiar el LED ──
function toggleLed(ledState) {
  // Enviamos el nuevo estado deseado (inverso del actual)
  const cmd = JSON.stringify({ cmd: "set", valor: ledState });
  ws.send(cmd);
  addMessageToLog(`Enviando → ${cmd}`, "send");
}

document
  .getElementById("luz1")
  .addEventListener("click", (e) => cambioEstado(e, "luz1"));

// Inicializar la aplicación
conectarWS();
window.onload = function () {
  initChart();
};
