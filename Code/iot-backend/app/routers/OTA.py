import os
import uuid
from typing import Optional

from fastapi import APIRouter, File, UploadFile, Form, HTTPException, status, Depends
from pydantic import BaseModel
from app.database import db, supabase_admin
from app.middleware.auth import get_current_device, get_current_user
from app.routers.mqtt import broker_host , broker_port
from app.services.mqtt_service import mqtt_service
from time import sleep
# --- Khởi tạo Router ---
router = APIRouter(
    prefix="/ota",
    tags=["OTA Management"]
)
class PostInfoOTA(BaseModel):
    client_id: str
    type: int # 0: check , 1: update
    
# --- Các mô hình dữ liệu Pydantic ---
class FileInfoBase(BaseModel):
    filename: str
    change_log: str
    version: str
    type: int # 0 : master , 1 : slave



class FileInfoResponse(FileInfoBase):
    id: str
    download_url: str
    created_at: str

# --- Các Endpoint của API ---

@router.post("/upload", status_code=status.HTTP_201_CREATED)
async def upload_file(
    file: UploadFile = File(..., description="File .bin cần tải lên"),
    filename: str = Form(..., description="Tên file"),
    change_log: str = Form(..., description="Ghi chú thay đổi"),
    version: str = Form(..., description="Phiên bản firmware"),
    type: int = Form(..., description="Loại firmware: 0 = master, 1 = slave"),
    current_user: dict = Depends(get_current_user)
):
    """
    Nhận một file và thông tin, lưu trữ vào Supabase Storage và Database.
    """
    try:
        # 1. Đọc nội dung file

        last_insert = db.execute_query(
            table="file_info",
            operation="select",
            filters={"user_id": current_user["id"] , "type": type}
        )
        if last_insert:
            last_insert = last_insert[0]
        else:
            last_insert = None
        if last_insert and last_insert["version"] >= version:
            return {
                "success": False,
                "message": "Vui lòng upload firmware mới hơn",
                "latest_version": last_insert["version"]
            }
      
        file_content = await file.read()
        
        # 2. Tạo một tên file duy nhất để tránh trùng lặp trên Supabase Storage
        file_id = uuid.uuid4()
        if type == 0:
            storage_path = f"master/{file_id}_{file.filename}"
        else:
            storage_path = f"slave/{file_id}_{file.filename}"


        # 3. Tải file lên Supabase Storage
        # Tham số: (path_on_storage, file_content)
        supabase_admin.storage.from_("firmwereStorge").upload(
            path=storage_path,
            file=file_content,
            file_options={"content-type": "application/octet-stream"} # Định dạng file .bin
        )
        download_url = supabase_admin.storage.from_("firmwereStorge").get_public_url(storage_path)
        # 4. Lưu thông tin metadata vào Supabase Database
        file_data_to_insert = {
            "filename": filename,
            "storage_path": storage_path,
            "download_url": download_url,
            "change_log": change_log,
            "version": version,
            "type": type,
            "user_id": current_user["id"]  # Lưu thông tin user upload
        }
        
        response = db.execute_query(
            table="file_info",
            operation="insert",
            data=file_data_to_insert
        )
        
        if not response or len(response) == 0:
            return {
                "success": False,
                "message": "Không thể lưu thông tin file vào database."
            }

        return {
            "success": True,
            "message": "File được tải lên thành công!",
            "data": response[0]
        }

    except Exception as e:
        # Xử lý các lỗi có thể xảy ra (ví dụ: lỗi kết nối, lỗi upload)
        print(f"Lỗi: {e}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Đã xảy ra lỗi trong quá trình xử lý: {e}"
        )

@router.get("/files", response_model=dict)
async def get_all_files(
    current_user: dict = Depends(get_current_user)
):
    """
    Lấy danh sách tất cả các file firmware của user hiện tại giành cho web admin
    """
    try:
        # Truy vấn tất cả file của user
        response = db.execute_query(
            table="file_info",
            operation="select",
            filters={"user_id": current_user["id"]}
        )

        if not response or len(response) == 0:
            return {
                "success": False,
                "message": "Không tìm thấy file nào"
            }
        
        return {
            "success": True,
            "data": response,
            "count": len(response)
        }

    except Exception as e:
        print(f"Lỗi: {e}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Đã xảy ra lỗi khi lấy danh sách file: {e}"
        )

@router.get("/get_info_update", response_model= dict)
async def get_info_update(
    current_device: dict = Depends(get_current_device)
):
    """
    Lấy thông tin update firmware mới nhất của user (để ESP32 tự động cập nhật OTA).
    """
    try:
        # Truy vấn file mới nhất
        response = db.execute_query(
            table="file_info",
            operation="select",
            filters={"user_id": current_device["user_id"]}
        )

        if not response:
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND,
                detail="Không tìm thấy firmware nào"
            )
        
        #Lấy 2 file loại master và slave mới nhất
        master_version = None
        slave_version = None
        master_link = None
        slave_link = None
        response = response[::-1]
        for file in response:
            if file["type"] == 0 and master_link is None:
                master_link = file["download_url"]
                master_version = file["version"]
            if file["type"] == 1 and slave_link is None:
                slave_link = file["download_url"]
                slave_version = file["version"]
        
        return {
            "success": True,
            "broker_server": broker_host,
            "broker_port": broker_port,
            "master_link": master_link,
            "master_version": master_version,
            "slave_link": slave_link,
            "slave_version": slave_version
        }

    except HTTPException:
        raise
    except Exception as e:
        print(f"Lỗi: {e}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Đã xảy ra lỗi khi lấy firmware mới nhất: {e}"
        )
@router.post("/check-info-ota")
def check_info_ota(data: PostInfoOTA , current_user: dict = Depends(get_current_user)):
    try:   
        result = db.execute_query(
            table="devices",
            operation="select",
            filters={"token_verify": data.client_id}
        )
        if not result or len(result) == 0:
            return {
                "success": False,
                "message": "Không tìm thấy thiết bị"
            }   
        if result[0]["user_id"] != current_user["id"]:
            return {
                "success": False,
                "message": "Không có thiết bị tương ứng , hãy đăng kí thiết bị để sử dụng"
            }
        if data.type == 0:
            mqtt_service.publish_message_NC(data.client_id, "OTA:CK")
            sleep(0.2)
            response = mqtt_service.client_ota_data.get(data.client_id)
            if response:
                return {
                "success": True,
                "data": response
            }
            else:
                return {
                    "success": False,
                    "message": "không có dữ liệu "
                }
        elif data.type == 1:
            mqtt_service.publish_message_NC(data.client_id, "OTA:UP")
            sleep(0.2)
            response = mqtt_service.client_ota_data.get(data.client_id)
            if response:
                return {
                "success": True,
                "data": response
            }
            else:
                return {
                    "success": False,
                    "message": "không có dữ liệu "
                }
    except Exception as e:
        print(f"Lỗi: {e}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Đã xảy ra lỗi khi lấy thông tin broker: {e}"
        )
@router.delete("/files/{file_id}", status_code=status.HTTP_200_OK)
async def delete_file(
    file_id: str,
    current_user: dict = Depends(get_current_user)
):
    """
    Xóa một file firmware.
    """
    try:
        # 1. Lấy thông tin file
        response = supabase_admin.table("file_info").select("*").eq("id", file_id).execute()

        if not response.data:
            raise HTTPException(
                status_code=status.HTTP_404_NOT_FOUND,
                detail=f"Không tìm thấy file với ID: {file_id}"
            )
        
        file_record = response.data[0]
        
        # 2. Kiểm tra quyền truy cập
        if file_record.get("user_id") != current_user["id"]:
            raise HTTPException(
                status_code=status.HTTP_403_FORBIDDEN,
                detail="Bạn không có quyền xóa file này"
            )
        
        # 3. Xóa file từ Storage
        supabase_admin.storage.from_("uploads").remove([file_record["storage_path"]])
        
        # 4. Xóa thông tin từ Database
        supabase_admin.table("file_info").delete().eq("id", file_id).execute()
        
        return {"message": "File đã được xóa thành công!", "file_id": file_id}

    except HTTPException:
        raise
    except Exception as e:
        print(f"Lỗi: {e}")
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail=f"Đã xảy ra lỗi khi xóa file: {e}"
        )
        