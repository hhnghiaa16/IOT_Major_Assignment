<<<<<<< HEAD
import threading
from typing import Any


=======
>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701
from fastapi import APIRouter, HTTPException, Depends, WebSocketDisconnect, status
from app.middleware.auth import get_current_device, get_current_user
from pydantic import BaseModel
from app.database import db
from fastapi import WebSocket
from concurrent.futures import ThreadPoolExecutor
from app.services.STTSystem import STTSystem
from app.services.mqtt_service import mqtt_service
from app.services.conversation_service import conversation_service
<<<<<<< HEAD
import httpx
=======
>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701
import asyncio

router = APIRouter(prefix="/audio_stream", tags=["Audio Stream"])
TAG = "AUDIO_STREAM"
class AudioStreamRequest(BaseModel):
    id : int 
    token_verify: str
    pin_label:str 
    device_name : str
    virtual_pin : int
executor_stt = ThreadPoolExecutor(max_workers=4, thread_name_prefix="STTSystem-Worker")
<<<<<<< HEAD
list_audio_url = dict[str, str]()
list_audio_url_lock = threading.Lock()
=======

>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701
@router.websocket("/ws/{client_id}")
async def websocket_endpoint(websocket: WebSocket, client_id: str):
    """
    WebSocket endpoint để nhận audio stream từ ESP32 và xử lý STT
    """
    await websocket.accept()
    print(f"{TAG} CONNECTION: Client {client_id} đã kết nối.")
    
    # Khởi tạo STT system cho client này
    stt_system = STTSystem(max_workers=1, token_master=client_id)
    chunk_count = 0
    
    try:
        while True:
            # Kiểm tra trạng thái voice từ MQTT
            if client_id in mqtt_service.client_is_voice and mqtt_service.client_is_voice[client_id]:
                # Client đang gửi audio
                data = await websocket.receive_bytes()
                chunk_count += 1
                
                # ✅ FIX: Xử lý audio chunk ĐÚNG CÁCH
                # Sử dụng asyncio.to_thread() để chạy sync function trong thread pool
                await asyncio.to_thread(stt_system.process_chunk, data)
<<<<<<< HEAD
            else:
                # Client ngừng gửi audio, lấy kết quả STT
                # print(f"{TAG} Client {client_id}: Kết thúc ghi âm, đang lấy kết quả...")
=======
                
                if chunk_count % 10 == 0:
                    print(f"{TAG} Client {client_id}: Đã nhận {chunk_count} chunks")
            else:
                # Client ngừng gửi audio, lấy kết quả STT
                print(f"{TAG} Client {client_id}: Kết thúc ghi âm, đang lấy kết quả...")
>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701
                
                text = stt_system.get_result_text()
                
                if not text or text.strip() == "":
                    text = "Không nhận diện được giọng nói"
                
<<<<<<< HEAD
                print(f"{TAG} STT Result: {text} + chunk_count: {chunk_count}")
=======
                print(f"{TAG} STT Result: {text}")
>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701
                
                # Gửi kết quả về client
                await websocket.send_text(text)
                await websocket.close()
                result = await conversation_service.chat(client_id, text)
<<<<<<< HEAD
                print(f"{TAG} [Client: {client_id}] Result: {result['response']}")
                final_audio_url = await get_audio_url_async(result['response'])
                with list_audio_url_lock:
                    list_audio_url[client_id] = final_audio_url.replace("https://", "http://")
                    mqtt_service.publish_message_NC(client_id, "AU:ON")
                print(f"{TAG} [Client: {client_id}] Final Audio URL: {final_audio_url}")
=======
                print(f"{TAG} [Client: {client_id}] Result: {result}")
>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701
                return
                
    except WebSocketDisconnect:
        print(f"{TAG} CONNECTION: Client {client_id} đã ngắt kết nối.")
        
        # Lấy kết quả cuối cùng nếu có
        final_transcript = stt_system.get_result_text()
        if final_transcript:
<<<<<<< HEAD
            print(f"{TAG} WEB SOCKET DISCONNECT FINAL TRANSCRIPT: {final_transcript}")
=======
            print(f"{TAG} FINAL TRANSCRIPT: {final_transcript}")
>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701
    
    except Exception as e:
        print(f"{TAG} ERROR: Lỗi với client {client_id}: {e}")
        import traceback
        traceback.print_exc()
<<<<<<< HEAD
        try:
            await websocket.close()
        except:
            pass
fpt_api_url = 'https://api.fpt.ai/hmi/tts/v5'
api_key = 'FQMvO5Pw87mBIYxO5oKwjsAuTY25b8sH'
async def get_audio_url_async(text: str):
    """
    Hàm bất đồng bộ (non-blocking) để gọi API của FPT,
    chờ file sẵn sàng bằng cách polling và trả về URL cuối cùng.
    """
    # Bước 1: Gửi yêu cầu tạo audio đến FPT.AI (bất đồng bộ)
    headers = {
        'api-key': api_key,
        'speed': '0',
        'voice': 'banmai',
        'format': 'wav'
    }
    
    # Sử dụng httpx.AsyncClient để thực hiện các yêu cầu HTTP bất đồng bộ
    async with httpx.AsyncClient() as client:
        try:
            response = await client.post(fpt_api_url, data=text.encode('utf-8'), headers=headers)
            response.raise_for_status() # Tự động raise lỗi nếu có vấn đề (4xx, 5xx)
        except httpx.RequestException as e:
            raise HTTPException(status_code=500, detail=f"Lỗi khi gọi FPT.AI: {e}")

        result = response.json()

        if result.get('error') != 0:
            raise HTTPException(status_code=400, detail=f"Lỗi từ FPT.AI: {result.get('message')}")

        async_url = result.get('async')
        if not async_url:
            raise HTTPException(status_code=500, detail="Không nhận được URL audio từ FPT.AI")

        # Bước 2: Polling (thăm dò) bất đồng bộ cho đến khi file audio sẵn sàng
        print(f"Server đang chờ file audio tại URL: {async_url}")
        max_attempts = 20  # Số lần thử tối đa
        attempt = 0
        
        while attempt < max_attempts:
            attempt += 1
            try:
                # Sử dụng HEAD request để kiểm tra sự tồn tại của file mà không tải về
                check_response = await client.head(async_url, timeout=10)
                
                if check_response.status_code == 200:
                    print("File audio đã sẵn sàng!")
                    return async_url # Trả về URL khi nó đã hợp lệ
                else:
                    print(f"Lần thử {attempt}/{max_attempts}: File chưa sẵn sàng (Status: {check_response.status_code}). Đợi 2 giây...")
                    # Dùng await asyncio.sleep() thay cho time.sleep() để không block server
                    await asyncio.sleep(2)
            except httpx.RequestError as e:
                print(f"Lỗi khi kiểm tra URL audio: {e}. Đợi 0.5 giây...")
                await asyncio.sleep(0.5)

        # Nếu sau max_attempts vẫn chưa sẵn sàng
        raise HTTPException(status_code=408, detail="File audio không sẵn sàng sau nhiều lần thử.")

@router.get("/get-audio-url")
async def get_audio_url_endpoint(client_id: str):
    """
    Endpoint này nhận văn bản, tạo audio và trả về URL để stream.
    Toàn bộ quá trình xử lý là bất đồng bộ.
    """
    # Lấy URL đã sẵn sàng từ FPT một cách bất đồng bộ
    final_audio_url = list_audio_url.get(client_id, None)
    if not final_audio_url:
        return {
            "success": False,
            "message": "Không tìm thấy URL audio"
        }
    else:
        return {
            "success": True,
            "audio_url": final_audio_url
        }
=======
        
        try:
            await websocket.close()
        except:
            pass
>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701
