# Auth schemas
# app/schemas/auth.py
from pydantic import BaseModel, EmailStr
from typing import Optional

class UserRegister(BaseModel):
    email: EmailStr
    name: str
    password: str
    type : int # 0 : admin , 1 : observer
<<<<<<< HEAD
class UserUpdate(BaseModel):
    name: str
    email: EmailStr
    # type : int # 0 : admin , 1 : observer
    password: str   
=======

>>>>>>> 5f2a5cdc6197e069a11939049c6819ea856af701
class UserLogin(BaseModel):
    email: EmailStr
    password: str

class Token(BaseModel):
    access_token: str
    token_type: str
    user: dict

class UserResponse(BaseModel):
    id: int
    email: str
    name: str
    is_active: bool
    created_at: str
    type : int # 0 : admin , 1 : observer

# app/schemas/device.py
