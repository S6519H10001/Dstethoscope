import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';
import 'package:audioplayers/audioplayers.dart';

void main() {
  runApp(AudioApp());
}

class AudioApp extends StatefulWidget {
  @override
  _AudioAppState createState() => _AudioAppState();
}

class _AudioAppState extends State<AudioApp> {
  final String apiUrl = 'http://YOUR_SERVER_IP:8000/list_sounds';
  List<dynamic> sounds = [];
  final AudioPlayer audioPlayer = AudioPlayer();

  void fetchSounds() async {
    var response = await http.get(Uri.parse(apiUrl));
    if (response.statusCode == 200) {
      setState(() {
        sounds = json.decode(response.body);
      });
    }
  }

  @override
  void initState() {
    super.initState();
    fetchSounds();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: Text("Lung Sound Recorder")),
        body: ListView.builder(
          itemCount: sounds.length,
          itemBuilder: (context, index) {
            return ListTile(
              title: Text(sounds[index]["timestamp"]),
              subtitle: Text("${sounds[index]["duration"]} sec"),
              trailing: IconButton(
                icon: Icon(Icons.play_arrow),
                onPressed: () => audioPlayer.play(sounds[index]["filename"]),
              ),
            );
          },
        ),
      ),
    );
  }
}
