// ============================================
// AUTH TYPES
// ============================================
export type AuthMode = 'login' | 'register';

export interface LoginRequest {
  email: string;
  password: string;
}

export interface RegisterRequest {
  email: string;
  password: string;
  name: string;
  type: number;
}

export interface RegisterViewerRequest {
  email: string;
  name: string;
  password: string;
}

export interface AuthResponse {
  access_token: string;
  user?: User;
  success?: boolean;
  message?: string;
}

export interface User {
  id: number;
  email: string;
  name: string;
  is_active?: boolean;
  type?: number; // 0 = Admin, 1 = Viewer (Guest)
}

// ============================================
// DEVICE TYPES
// ============================================
export type DeviceType = 'MASTER' | 'SLAVE' | 'GUEST';
export type ConnectionStatus = 'ONLINE' | 'OFFLINE';

export interface Device {
  id: number;
  device_name: string;
  device_type: DeviceType;
  device_access_token?: string;
  device_token?: string;
  connection_status?: ConnectionStatus;
  is_online?: boolean;
  is_active?: boolean;
  last_seen?: string;
  last_seen_at?: string;
  created_at?: string;
  registered_at?: string;
  token_verify?: string;
}

export interface DeviceListResponse {
  success: boolean;
  devices?: Device[];
  message?: string;
}

export interface DeviceInfoResponse {
  success: boolean;
  device?: Device[];
  message?: string;
}

export interface RegisterDeviceRequest {
  device_name: string;
  device_type: 'MASTER' | 'SLAVE';
}

export interface RegisterDeviceResponse {
  success: boolean;
  message?: string;
  device?: Device;
}

// ============================================
// CONFIG PIN TYPES
// ============================================
export type PinType = 'INPUT' | 'OUTPUT';
export type DataType = 'string' | 'float' | 'integer' | 'boolean' | 'datetime' | 'json';

export interface ConfigPin {
  device_token?: string;
  virtual_pin: number;
  pin_label: string;
  pin_type: PinType;
  data_type: DataType;
  ai_keywords?: string;
}

export interface ConfigPinListResponse {
  success: boolean;
  config_pins?: ConfigPin[];
  message?: string;
}

export interface ConfigPinRequest {
  device_token: string;
  pins: ConfigPin[];
}

export interface ConfigPinResponse {
  success: boolean;
  message?: string;
}

// ============================================
// DASHBOARD TYPES
// ============================================
export interface DashboardBlock {
  id: number;
  user_id: number;
  created_at: string;
  typeblock: number;
  label_block: string;
  virtual_pin: number;
  device_name: string;
  pin_label: string;
  token_verify: string;
  value?: number;
}

export interface DashboardBlocksResponse {
  success: boolean;
  blocks?: DashboardBlock[];
  message?: string;
}

export interface CreateBlockRequest {
  id: number;
  device_name: string;
  token_verify: string;
  pin_label: string;
  virtual_pin: number;
  label_block: string;
  type_block: number;
}

export interface CreateBlockResponse {
  success: boolean;
  message?: string;
}

// ============================================
// MQTT TYPES
// ============================================
export interface DeviceCommandRequest {
  token_verify: string;
  virtual_pin: number;
  value: number;
}

export interface DeviceCommandResponse {
  success: boolean;
  message?: string;
}

// ============================================
// SENSOR TYPES
// ============================================
export interface SensorData {
  id: number;
  virtual_pin: number;
  value_numeric: number | null;
  value_string: string;
  timestamp: string;
  token_verify: string;
}

export interface SensorDataResponse {
  success?: boolean;
  data?: SensorData[];
  message?: string;
}

// ============================================
// COMPONENT PROPS TYPES
// ============================================
export interface HeaderProps {
  currentPage: string;
  onNavigate: (page: string) => void;
  onLogout?: () => void;
}

export interface LoginProps {
  onLogin?: (user?: User) => void;
}

export interface MyDevicesProps {
  onClose: () => void;
}

export interface DeviceConfigModalProps {
  device: {
    id: number;
    device_name: string;
    device_token?: string;
    device_access_token?: string;
  };
  onClose: () => void;
}

export interface BlockConfigModalProps {
  block: DashboardBlock | null;
  isNewBlock: boolean;
  blockType: number;
  onClose: () => void;
  onSave: () => void;
}

export interface ChartBlockProps {
  block: DashboardBlock;
  onConfigure: () => void;
  onDelete: () => void;
}

// ============================================
// INTERNAL TYPES (for hooks/services)
// ============================================
export interface BlockConfigDevice {
  id: number;
  device_name: string;
  device_token: string;
  token_verify: string;
}

export interface BlockConfigPin {
  virtual_pin: number;
  pin_label: string;
  pin_type: string;
  data_type?: string;
}

