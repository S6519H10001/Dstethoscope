import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:web_socket_channel/io.dart';
import 'package:audioplayers/audioplayers.dart';
import 'package:http/http.dart' as http;
import 'ai.dart';

class AudioApp extends StatefulWidget {
  @override
  _AudioAppState createState() => _AudioAppState();
}

class _AudioAppState extends State<AudioApp> {
  final String wsUrl = 'ws://your-server-ip:8000/audio';
  final String startRecordingUrl = 'http://your-server-ip:8000/start-recording';
  final String stopRecordingUrl = 'http://your-server-ip:8000/stop-recording';
  final String downloadUrl = 'http://your-server-ip:8000/download-audio';

  late IOWebSocketChannel channel;
  final AudioPlayer player = AudioPlayer();
  bool isRecording = false;
  List<double> waveData = [];

  @override
  void initState() {
    super.initState();
    connectWebSocket();
  }

  void connectWebSocket() {
    channel = IOWebSocketChannel.connect(wsUrl);
    channel.stream.listen((data) {
      player.play(BytesSource(data));
      updateWaveform(data);
    });
  }

  void updateWaveform(Uint8List audioData) {
    List<double> amplitudes = audioData.map((e) => e / 255.0).toList();
    setState(() {
      waveData = amplitudes;
    });
  }

  Future<void> startRecording() async {
    await http.post(Uri.parse(startRecordingUrl));
    setState(() => isRecording = true);
  }

  Future<void> stopRecording() async {
    await http.post(Uri.parse(stopRecordingUrl));
    setState(() => isRecording = false);
  }

  Future<void> downloadAudio() async {
    var response = await http.get(Uri.parse(downloadUrl));
    if (response.statusCode == 200) {
      File file = File('/storage/emulated/0/Download/recorded_audio.wav');
      await file.writeAsBytes(response.bodyBytes);
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text("📥 ดาวน์โหลดเสียงสำเร็จ!")));
    }
  }

  @override
  void dispose() {
    channel.sink.close();
    player.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text("Live Audio Streaming")),
      body: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          ElevatedButton(
            onPressed: isRecording ? null : startRecording,
            child: Text("🎬 Start Recording"),
          ),
          ElevatedButton(
            onPressed: isRecording ? stopRecording : null,
            child: Text("🛑 Stop Recording"),
          ),
          ElevatedButton(
            onPressed: downloadAudio,
            child: Text("📥 Download Audio"),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.push(context, MaterialPageRoute(builder: (context) => RecordedAudioPage()));
            },
            child: Text("🎧 ฟังเสียงที่บันทึกไว้ & วิเคราะห์ด้วย AI"),
          ),
          SizedBox(height: 20),
          Text("🎵 Waveform Display"),
          Expanded(child: WaveformWidget(waveData)),
        ],
      ),
    );
  }
}

class WaveformWidget extends StatelessWidget {
  final List<double> waveData;

  WaveformWidget(this.waveData);

  @override
  Widget build(BuildContext context) {
    return CustomPaint(
      painter: WaveformPainter(waveData),
      size: Size(double.infinity, 150),
    );
  }
}

class WaveformPainter extends CustomPainter {
  final List<double> waveData;

  WaveformPainter(this.waveData);

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = Colors.blue
      ..strokeWidth = 2
      ..strokeCap = StrokeCap.round;

    double width = size.width / waveData.length;
    for (int i = 0; i < waveData.length; i++) {
      double x = i * width;
      double y = (1 - waveData[i]) * size.height;
      canvas.drawLine(Offset(x, size.height / 2), Offset(x, y), paint);
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}
