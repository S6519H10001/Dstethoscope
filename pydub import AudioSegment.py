from pydub import AudioSegment
from pydub.playback import play

audio = AudioSegment.from_wav("fastapi_audio.wav")
play(audio)
