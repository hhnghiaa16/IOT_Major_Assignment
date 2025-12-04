"""
Device Control Tools - Các công cụ điều khiển thiết bị IoT
"""
from app.services.tool_service import registry
from app.services.mqtt_service import mqtt_service
from app.database import db
<<<<<<< HEAD
from app.routers.devices import get_slave_device
import time
import json
@registry.register("tool lấy danh sách users trong hệ thống\
")
=======
import time
import json

# ==================== DEVICE CONTROL TOOLS ====================

# @registry.register("Bật thiết bị IoT theo tên thiết bị")
# def turn_on_device(device_name: str):
#     """
#     Bật thiết bị IoT
    
#     Args:
#         device_name: Tên thiết bị (ví dụ: "Đèn phòng khách", "Quạt phòng ngủ")
    
#     Returns:
#         Kết quả điều khiển
#     """
#     try:
#         # Tìm device trong database
#         device = db.devices.find_one({"name": device_name})
        
#         if not device:
#             return f"Không tìm thấy thiết bị '{device_name}'"
        
#         device_id = device.get("device_id")
#         topic = f"device/{device_id}/control"
        
#         # Gửi lệnh MQTT
#         mqtt_service.publish(topic, json.dumps({
#             "action": "turn_on",
#             "timestamp": time.time()
#         }))
        
#         return f"Đã bật thiết bị '{device_name}' thành công"
        
#     except Exception as e:
#         return f"Lỗi khi bật thiết bị: {str(e)}"


# @registry.register("Tắt thiết bị IoT theo tên thiết bị")
# def turn_off_device(device_name: str):
#     """
#     Tắt thiết bị IoT
    
#     Args:
#         device_name: Tên thiết bị
    
#     Returns:
#         Kết quả điều khiển
#     """
#     try:
#         device = db.devices.find_one({"name": device_name})
        
#         if not device:
#             return f"Không tìm thấy thiết bị '{device_name}'"
        
#         device_id = device.get("device_id")
#         topic = f"device/{device_id}/control"
        
#         mqtt_service.publish(topic, json.dumps({
#             "action": "turn_off",
#             "timestamp": time.time()
#         }))
        
#         return f"Đã tắt thiết bị '{device_name}' thành công"
        
#     except Exception as e:
#         return f"Lỗi khi tắt thiết bị: {str(e)}"


# @registry.register("Điều chỉnh độ sáng đèn")
# def set_brightness(device_name: str, brightness: int):
#     """
#     Điều chỉnh độ sáng của đèn
    
#     Args:
#         device_name: Tên thiết bị đèn
#         brightness: Độ sáng từ 0-100
    
#     Returns:
#         Kết quả điều chỉnh
#     """
#     try:
#         if not 0 <= brightness <= 100:
#             return "Độ sáng phải trong khoảng 0-100"
        
#         device = db.devices.find_one({"name": device_name})
        
#         if not device:
#             return f"Không tìm thấy thiết bị '{device_name}'"
        
#         device_id = device.get("device_id")
#         topic = f"device/{device_id}/control"
        
#         mqtt_service.publish(topic, json.dumps({
#             "action": "set_brightness",
#             "value": brightness,
#             "timestamp": time.time()
#         }))
        
#         return f"Đã điều chỉnh độ sáng '{device_name}' thành {brightness}%"
        
#     except Exception as e:
#         return f"Lỗi khi điều chỉnh độ sáng: {str(e)}"


# # ==================== SENSOR READING TOOLS ====================

# @registry.register("Đọc nhiệt độ từ cảm biến")
# def read_temperature(sensor_name: str):
#     """
#     Đọc giá trị nhiệt độ từ cảm biến
    
#     Args:
#         sensor_name: Tên cảm biến (ví dụ: "Cảm biến phòng khách")
    
#     Returns:
#         Giá trị nhiệt độ hiện tại
#     """
#     try:
#         # Lấy giá trị sensor mới nhất từ database
#         sensor_data = db.sensors.find_one(
#             {"sensor_name": sensor_name, "type": "temperature"},
#             sort=[("timestamp", -1)]
#         )
        
#         if not sensor_data:
#             return f"Không tìm thấy dữ liệu cảm biến '{sensor_name}'"
        
#         temp = sensor_data.get("value")
#         timestamp = sensor_data.get("timestamp")
        
#         return f"Nhiệt độ tại '{sensor_name}' là {temp}°C (cập nhật lúc {timestamp})"
        
#     except Exception as e:
#         return f"Lỗi khi đọc nhiệt độ: {str(e)}"


# @registry.register("Đọc độ ẩm từ cảm biến")
# def read_humidity(sensor_name: str):
#     """
#     Đọc giá trị độ ẩm từ cảm biến
    
#     Args:
#         sensor_name: Tên cảm biến
    
#     Returns:
#         Giá trị độ ẩm hiện tại
#     """
#     try:
#         sensor_data = db.sensors.find_one(
#             {"sensor_name": sensor_name, "type": "humidity"},
#             sort=[("timestamp", -1)]
#         )
        
#         if not sensor_data:
#             return f"Không tìm thấy dữ liệu cảm biến '{sensor_name}'"
        
#         humidity = sensor_data.get("value")
#         timestamp = sensor_data.get("timestamp")
        
#         return f"Độ ẩm tại '{sensor_name}' là {humidity}% (cập nhật lúc {timestamp})"
        
#     except Exception as e:
#         return f"Lỗi khi đọc độ ẩm: {str(e)}"


# # ==================== QUERY TOOLS ====================

# @registry.register("Liệt kê tất cả thiết bị")
# def list_all_devices():
#     """
#     Liệt kê tất cả thiết bị IoT đang kết nối
    
#     Returns:
#         Danh sách thiết bị
#     """
#     try:
#         devices = list(db.devices.find({}, {"_id": 0, "name": 1, "status": 1, "type": 1}))
        
#         if not devices:
#             return "Không có thiết bị nào"
        
#         result = "Danh sách thiết bị:\n"
#         for idx, device in enumerate(devices, 1):
#             name = device.get("name", "Unknown")
#             status = device.get("status", "offline")
#             device_type = device.get("type", "unknown")
#             result += f"{idx}. {name} ({device_type}) - {status}\n"
        
#         return result
        
#     except Exception as e:
#         return f"Lỗi khi liệt kê thiết bị: {str(e)}"
@registry.register("Tool Bật đèn nhà. Sử dụng tool này khi người dùng yêu cầu bật đèn nhà")
def turn_on_device_home():
    """
    Bật đèn nhà - Thực hiện hành động bật đèn trong nhà
    """
    return {"message": "Đã bật đèn nhà thành công", "success": True}

@registry.register("Tool Bật đèn bếp. Sử dụng tool này khi người dùng yêu cầu bật đèn bếp")
def turn_on_device_kitchen():
    """
    Bật đèn bếp - Thực hiện hành động bật đèn trong bếp
    """
    return {"message": "Đã bật đèn bếp thành công", "success": True}

@registry.register("Tool Mở cửa. Sử dụng tool này khi người dùng yêu cầu mở cửa.")
def open_the_door():
    """
    Mở cửa - Thực hiện hành động mở cửa
    """
    return {"message": "Đã mở cửa thành công", "success": True}
@registry.register("tool lấy danh sách các người dùng , các user trong hệ thống ")
>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701
def get_list_users():
    """
    Láy danh sách các user trong database
    """
    users = db.execute_query(
        table="users",
        operation="select",
        filters={"limit": 10}
    )
    return {
        "message": "Danh sách các user thành công ",
        "users": users,
        "success": True
    }
<<<<<<< HEAD
@registry.register(
    "Dùng khi người dùng yêu cầu kích hoạt thiết bị hoặc lấy dữ liệu  \
    Tool này trả về danh sách AI keywords của các slave thuộc master. \
    Nếu người dùng yêu cầu kích hoạt thiết bị thì is_ouput = true \
    Nếu người dùng yêu cầu lấy dữ liệu từ thiết bị slave thuộc master thì is_ouput = false \
    Kết quả này thường được dùng tiếp cho tool control_device_slave khi người dùng yêu cầu bật thiết bị nào đó."
)
def get_list_ai_keywords(master_token_verify: str , is_ouput : bool = False):
    """
    Lấy danh sách AI keywords của các thiết bị slave thuộc thiết bị master
    """
    devices = db.execute_query(
        table="devices",
        operation="select",
        filters={"token_verify": master_token_verify}
    )
    if devices:
        user_id = devices[0]["user_id"]
    else :
        return {
            "success": False,
            "message": "Không tìm thấy thiết bị master",
        }
    devices = db.execute_query(
        table="devices",
        operation="select",
        filters={"user_id": user_id, "device_type": "SLAVE"}
    )
    if devices:
        list_device_slave_with_ai_keywords = []
        for device in devices:
            device_pins = db.execute_query(
                table="device_pins",
                operation="select",
                filters={"device_token": device["device_token"] , "pin_type": "OUTPUT" if is_ouput else "INPUT"}
            )
            if device_pins:
                list_device_slave_with_ai_keywords.append({
                    "token_verify": device["token_verify"],
                    "list_device_slave_with_ai_keywords": device_pins,
                })
        if list_device_slave_with_ai_keywords:
            return {
                "success": True,
                "message": "Lấy danh sách AI keywords của các thiết bị slave thành công ",
                "list_device_slave_with_ai_keywords": list_device_slave_with_ai_keywords,
            }
        else :
            return {
                "success": False,
                "message": "Các thiết bị slave thuộc thiết bị master này không có AI keywords nào cả",
            }
    else :
        return {
            "success": False,
            "message": "Thiết bị master này không có slave nào cả , hãy đăng kí để sử dụng",
        }

@registry.register(
    "Dùng khi người dùng muốn điều khiển thiết bị slave. \
    Tool này nhận token_verify, virtual_pin OUTPUT và giá trị float. \
    Thường được gọi sau khi dùng get_list_ai_keywords với is_output = true\
")
def control_device_slave(token_verify: str, virtual_pin: int, value: float):
    """
    Điều khiển thiết bị slave bằng token_verify của thiết bị muốn điều kiển cộng với virtual_pin loại OUTTPUT của chân điều khiển
    """
    device = db.execute_query(
        table="devices",
        operation="select",
        filters={"token_verify": token_verify}
    )
    if not device:
        return {
            "success": False,
            "message": "Thiết bị Slave này không tồn tại",
        }
    if device[0]["device_type"] == "MASTER":
        return {
            "success": False,
            "message": "Chỉ thiết bị Slave mới có thể bị điều khiển ",
        }
    value = str(value)
    res = mqtt_service.publish_message_CT(token_verify, virtual_pin, value)
    if res is None:
        return {
            "success": True,
            "message": "Đã gửi lệnh điều khiển thành công",
        }
    else :
        return {
            "success": False,
            "message": res,
        }
@registry.register(
    "Dùng khi người dùng muốn lấy dữ liệu từ các thiết bị ví dụ như : độ ẩm , nhiệt độ , ánh sáng , nồng độ , ... \
    Tool này nhận token_verify của các thiết bị slave, virtual_pin loại INPUT và giá trị int là số lượng giá trị muốn lấy  \
    Nếu limit là 0 thì lấy tất cả dữ liệu từ chân thu thập dữ liệu đã chọn \
    Thường được gọi sau khi dùng get_list_ai_keywords với is_output = false\
")
def get_device_data(token_verify: str, virtual_pin: int, limit: int):
    """
    Lấy dữ liệu từ các chân thu thập dữ liệu của các slave thuộc thiết bị master
    """
    device = db.execute_query(
        table="devices",
        operation="select",
        filters={"token_verify": token_verify}
    )
    if not device:
        return {
            "success": False,
            "message": "Thiết bị Slave này không tồn tại",
        }
    if device[0]["device_type"] == "MASTER":
        return {
            "success": False,
            "message": "Chỉ thiết bị Slave mới có thể bị điều khiển ",
        }
    sensor_data = db.execute_query(
        table="sensor_data",
        operation="select",
        filters={"token_verify": token_verify, "virtual_pin": virtual_pin}
    )
    if not sensor_data:
        return {
            "success": False,
            "message": "Không tìm thấy dữ liệu từ chân thu thập dữ liệu này",
        }   
    
    if limit == 0:
        return {
            "success": True,
            "message": "Lấy dữ liệu từ chân thu thập dữ liệu thành công ",
            "sensor_data": sensor_data[-(min(limit, 100)):],
            "count": len(sensor_data[-(min(limit, 100)):])
        }
    else :
        return {
            "success": True,
            "message": "Lấy dữ liệu từ chân thu thập dữ liệu thành công ",
            "sensor_data": sensor_data[-(min(limit, 100)):],
            "count": len(sensor_data[-(min(limit, 100)):])
        }
=======
# @registry.register("Kiểm tra trạng thái thiết bị")
# def check_device_status(device_name: str):
#     """
#     Kiểm tra trạng thái của một thiết bị cụ thể
    
#     Args:
#         device_name: Tên thiết bị cần kiểm tra
    
#     Returns:
#         Trạng thái thiết bị
#     """
#     try:
#         device = db.devices.find_one({"name": device_name})
        
#         if not device:
#             return f"Không tìm thấy thiết bị '{device_name}'"
        
#         status = device.get("status", "unknown")
#         device_type = device.get("type", "unknown")
#         last_seen = device.get("last_seen", "N/A")
        
#         return f"Thiết bị '{device_name}' ({device_type}) đang {status}, lần cuối kết nối: {last_seen}"
        
#     except Exception as e:
#         return f"Lỗi khi kiểm tra trạng thái: {str(e)}"


# # ==================== SCHEDULE TOOLS (Nâng cao) ====================

# @registry.register("Hẹn giờ bật thiết bị")
# def schedule_turn_on(device_name: str, delay_seconds: int):
#     """
#     Hẹn giờ bật thiết bị sau một khoảng thời gian
    
#     Args:
#         device_name: Tên thiết bị
#         delay_seconds: Số giây chờ trước khi bật
    
#     Returns:
#         Kết quả hẹn giờ
#     """
#     try:
#         # Giả lập delay (trong thực tế nên dùng scheduler như Celery)
#         time.sleep(delay_seconds)
        
#         # Sau đó gọi turn_on_device
#         return turn_on_device(device_name)
        
#     except Exception as e:
#         return f"Lỗi khi hẹn giờ: {str(e)}"

>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701

print("✅ Đã load Device Tools thành công!")

