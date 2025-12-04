"""
Device Control Tools - Các công cụ điều khiển thiết bị IoT
"""
from app.services.tool_service import registry
from app.services.mqtt_service import mqtt_service
from app.database import db
from app.routers.devices import get_slave_device
import time
import json
@registry.register("tool lấy danh sách users trong hệ thống\
")
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

print("✅ Đã load Device Tools thành công!")

