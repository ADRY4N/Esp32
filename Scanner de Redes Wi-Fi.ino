#include <WiFi.h>
#include <WebServer.h>
#include <vector>
#include <map>

WebServer server(80);

struct WiFiNetwork {
  String ssid;
  String bssid;
  int rssi;
  int channel;
  String encryption;
  bool isHidden;
  String vendor;
};

std::vector<WiFiNetwork> networks;

// Mapeamento básico de prefixos OUI para nomes de fabricantes
std::map<String, String> vendorMap = {
  {"84:F3:EB", "TP-Link"},
  {"A4:2B:B0", "Intel"},
  {"F4:F5:D8", "Samsung"},
  {"B8:27:EB", "Raspberry Pi"},
  {"00:1A:2B", "Cisco"},
  {"E8:94:F6", "Apple"},
  {"EC:41:18", "Xiaomi"},
  {"D8:67:D9", "Huawei"},
  {"3C:84:6A", "Asus"},
  {"D0:37:45", "LG"},
  {"00:1C:B3", "Dell"},
  {"00:26:B0", "Sony"},
  {"00:0C:29", "VMware"},
  {"00:50:F2", "Microsoft"},
  {"00:1F:16", "Nokia"},
  {"00:23:12", "Motorola"},
  {"00:1D:0F", "Acer"},
  {"00:24:D6", "Siemens"},
  {"00:1E:68", "HTC"},
  {"00:1F:3A", "ASRock"},
  {"00:1A:11", "RIM (BlackBerry)"},
  {"00:1B:63", "HP"},
  {"00:1E:EC", "Hon Hai (Foxconn)"},
  {"00:1F:5B", "Lenovo"},
  {"00:21:5A", "Netgear"},
  {"00:22:5F", "ZTE"},
  {"00:25:BC", "LG Electronics"},
  {"00:26:5E", "Belkin"},
  {"00:15:E9", "Google"},
  {"00:12:0E", "Toshiba"},
  {"00:0D:4B", "Buffalo"},
  {"00:0E:6D", "Sagemcom"},
  {"00:0F:B0", "Realtek"},
  {"00:11:22", "BenQ"},
  {"00:13:D4", "D-Link"},
  {"00:14:51", "Texas Instruments"},
  {"00:16:6C", "Infineon Technologies"},
  {"00:17:9A", "NEC"},
  {"00:18:8B", "Brother Industries"},
  {"00:19:07", "Alcatel-Lucent"},
  {"00:01:4A", "Alcatel-Lucent (Nokia)"},
  {"00:10:7B", "Ciena"},
  {"00:0F:64", "Huawei"},
  {"00:19:A6", "ZTE"},
  {"00:1E:40", "Ericsson"},
  {"00:1E:C9", "FiberHome"},
  {"00:25:9E", "ADTRAN"},
  {"00:26:88", "Calix"},
  {"00:30:AB", "Cisco"},
  {"00:0B:5F", "Juniper Networks"},
  {"00:1B:21", "NEC"},
  {"00:1C:F0", "Tellabs"},
  {"00:1F:9F", "MikroTik"},
  {"00:23:8C", "Motorola (ARRIS/CommScope)"},
  {"00:24:36", "DASAN Zhone"},
  {"00:0C:EE", "UTStarcom"},
  {"00:0E:FC", "Allied Telesis"},
  {"00:10:94", "Corning Optical Communications"},
  {"00:0C:42", "MikroTik"},
  {"50:C7:BF", "TP-Link"}
};

String getVendor(const String& bssid) {
  String prefix = bssid.substring(0, 8);
  prefix.toUpperCase();
  if (vendorMap.count(prefix)) {
    return vendorMap[prefix];
  }
  return "Desconhecido";
}

String encryptionType(wifi_auth_mode_t encryption) {
  switch (encryption) {
    case WIFI_AUTH_OPEN: return "Aberta";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    case WIFI_AUTH_WPA3_PSK: return "WPA3-PSK";
    default: return "Desconhecida";
  }
}

void scanNetworks() {
  networks.clear();
  int n = WiFi.scanNetworks(false, true);
  for (int i = 0; i < n; i++) {
    WiFiNetwork net;
    net.ssid = WiFi.SSID(i);
    net.isHidden = (net.ssid.length() == 0);
    if (net.isHidden) net.ssid = "Rede Oculta";
    net.bssid = WiFi.BSSIDstr(i);
    net.rssi = WiFi.RSSI(i);
    net.channel = WiFi.channel(i);
    net.encryption = encryptionType(WiFi.encryptionType(i));
    net.vendor = getVendor(net.bssid);
    networks.push_back(net);
  }
  WiFi.scanDelete();
}

String generateHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='utf-8'>
  <title>Scanner Wi-Fi ESP32</title>
  <style>
    table { border-collapse: collapse; width: 100%; }
    th, td { border: 1px solid #ccc; padding: 8px; text-align: left; }
    th { background: #f2f2f2; }
    body { font-family: Arial; }
    canvas { max-width: 100%; height: 300px; }
  </style>
</head>
<body>
  <h2>Redes Wi-Fi Detectadas</h2>
  <table id="wifiTable">
    <thead><tr><th>SSID</th><th>BSSID</th><th>RSSI</th><th>Canal</th><th>Criptografia</th><th>Oculta</th><th>Fabricante</th></tr></thead>
    <tbody></tbody>
  </table>

  <h3>Gráfico de Ocupação de Canais</h3>
  <canvas id="channelChart"></canvas>

  <script>
    class Chart {
      constructor(canvas, config) {
        this.ctx = canvas.getContext('2d');
        this.data = config.data;
        this.type = config.type;
        this.draw();
      }
      draw() {
        const ctx = this.ctx;
        const labels = this.data.labels;
        const data = this.data.datasets[0].data;
        const width = ctx.canvas.width;
        const height = ctx.canvas.height;
        ctx.clearRect(0, 0, width, height);
        const barWidth = width / labels.length * 0.6;
        const maxData = Math.max(...data, 1);
        const scale = height / maxData * 0.8;
        ctx.fillStyle = 'rgba(54, 162, 235, 0.6)';
        ctx.strokeStyle = 'rgba(54, 162, 235, 1)';
        ctx.lineWidth = 1;
        ctx.font = '12px Arial';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'bottom';
        for (let i = 0; i < labels.length; i++) {
          const barHeight = data[i] * scale;
          const x = i * (width / labels.length) + (width / labels.length - barWidth) / 2;
          const y = height - barHeight - 20;
          ctx.fillRect(x, y, barWidth, barHeight);
          ctx.strokeRect(x, y, barWidth, barHeight);
          ctx.fillStyle = '#000';
          ctx.fillText(labels[i], x + barWidth / 2, height - 5);
          ctx.fillText(data[i], x + barWidth / 2, y - 5);
          ctx.fillStyle = 'rgba(54, 162, 235, 0.6)';
        }
      }
      update() { this.draw(); }
    }

    let chart;

    function updateChart(data) {
      const channelCounts = {};
      data.forEach(net => {
        channelCounts[net.channel] = (channelCounts[net.channel] || 0) + 1;
      });
      const labels = Object.keys(channelCounts).sort((a,b) => a-b);
      const values = labels.map(l => channelCounts[l]);
      if (!chart) {
        const ctx = document.getElementById('channelChart');
        chart = new Chart(ctx, {
          type: 'bar',
          data: { labels: labels, datasets: [{ label: 'Redes por Canal', data: values }] }
        });
      } else {
        chart.data.labels = labels;
        chart.data.datasets[0].data = values;
        chart.update();
      }
    }

    function updateTable(data) {
      const tbody = document.querySelector('#wifiTable tbody');
      tbody.innerHTML = '';
      data.forEach(net => {
        const row = `<tr>
          <td>${net.ssid}</td>
          <td>${net.bssid}</td>
          <td>${net.rssi} dBm</td>
          <td>${net.channel}</td>
          <td>${net.encryption}</td>
          <td>${net.hidden ? 'Sim' : 'Não'}</td>
          <td>${net.vendor}</td>
        </tr>`;
        tbody.innerHTML += row;
      });
    }

    async function fetchData() {
      const res = await fetch('/data');
      const data = await res.json();
      updateTable(data);
      updateChart(data);
    }

    setInterval(fetchData, 5000);
    fetchData();
  </script>
</body>
</html>
)rawliteral";
  return html;
}

void handleRoot() {
  String html = generateHTML();
  server.send(200, "text/html", html);
}

void handleData() {
  scanNetworks();
  String json = "[";
  for (size_t i = 0; i < networks.size(); ++i) {
    const auto& net = networks[i];
    json += "{";
    json += "\"ssid\":\"" + net.ssid + "\"," +
            "\"bssid\":\"" + net.bssid + "\"," +
            "\"rssi\":" + String(net.rssi) + "," +
            "\"channel\":" + String(net.channel) + "," +
            "\"encryption\":\"" + net.encryption + "\"," +
            "\"hidden\":" + String(net.isHidden ? "true" : "false") + "," +
            "\"vendor\":\"" + net.vendor + "\"";
    json += "}";
    if (i < networks.size() - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  
  // Configura o ESP32 como Access Point para que você possa se conectar nele
  WiFi.mode(WIFI_AP_STA);
  bool apStarted = WiFi.softAP("ESP32_Scanner", "12345678");
  if (apStarted) {
    Serial.println("Access Point iniciado com SSID: ESP32_Scanner");
    Serial.print("IP do AP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Falha ao iniciar Access Point");
  }

  WiFi.disconnect(); // Garante que o STA não esteja conectado

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  server.handleClient();
}
