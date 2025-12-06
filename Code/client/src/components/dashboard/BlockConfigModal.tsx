/**
 * BlockConfigModal Component (Dumb Component)
 * Pure UI component for block configuration
 */

import React from 'react';
import '../../styles/BlockConfigModal.css';
import { useBlockConfig } from '../../hooks/useBlockConfig';
import type { BlockConfigModalProps } from '../../types';

const BlockConfigModal: React.FC<BlockConfigModalProps> = ({
  block,
  isNewBlock,
  blockType,
  onClose,
  onSave,
}) => {
  const {
    devices,
    pins,
    loading,
    formData,
    setFormData,
    error,
    saving,
    handleDeviceChange,
    handlePinChange,
    handleSubmit,
  } = useBlockConfig({ block, isNewBlock, blockType });

  const handleFormSubmit = async (e: React.FormEvent) => {
    await handleSubmit(e);
    if (!error) {
      onSave();
    }
  };

  return (
    <div className="block-config-modal-overlay" onClick={onClose}>
      <div className="block-config-modal" onClick={(e) => e.stopPropagation()}>
        <div className="block-config-modal-header">
          <h2>{isNewBlock ? 'Thêm block mới' : 'Cấu hình block'}</h2>
          <button className="modal-close-btn" onClick={onClose}>
            ×
          </button>
        </div>

        <div className="block-config-modal-content">
          {loading ? (
            <div className="loading-state">Đang tải...</div>
          ) : (
            <form onSubmit={handleFormSubmit} className="block-config-form">
              <div className="form-group">
                <label className="form-label">
                  <span>Loại block</span>
                </label>
                <div className="block-type-display">
                  {formData.type_block === 0 ? 'Nút bật/tắt' : 'Biểu đồ'}
                </div>
              </div>

              <div className="form-group">
                <label className="form-label">
                  <span>Tên block</span>
                  <span className="required-indicator">*</span>
                </label>
                <input
                  className="form-input"
                  type="text"
                  value={formData.label_block}
                  onChange={(e) => setFormData((prev) => ({ ...prev, label_block: e.target.value }))}
                  placeholder="Ví dụ: Bật đèn phòng khách"
                  disabled={saving}
                />
              </div>

              <div className="form-group">
                <label className="form-label">
                  <span>Chọn thiết bị</span>
                  <span className="required-indicator">*</span>
                </label>
                <select
                  className="form-input"
                  value={formData.device_name}
                  onChange={(e) => handleDeviceChange(e.target.value)}
                  disabled={saving}
                >
                  <option value="">-- Chọn thiết bị --</option>
                  {devices.map((device) => (
                    <option key={device.id} value={device.device_name}>
                      {device.device_name}
                    </option>
                  ))}
                </select>
              </div>

              {formData.device_name && pins.length > 0 && (
                <div className="form-group">
                  <label className="form-label">
                    <span>
                      {formData.type_block === 0
                        ? 'Chọn pin điều khiển (OUTPUT)'
                        : 'Chọn chân cảm biến (INPUT)'}
                    </span>
                    <span className="required-indicator">*</span>
                  </label>
                  <select
                    className="form-input"
                    value={formData.virtual_pin}
                    onChange={(e) => handlePinChange(Number(e.target.value))}
                    disabled={saving}
                  >
                    <option value={0}>
                      {formData.type_block === 0 ? '-- Chọn pin OUTPUT --' : '-- Chọn pin INPUT --'}
                    </option>
                    {pins.map((pin) => (
                      <option key={pin.virtual_pin} value={pin.virtual_pin}>
                        Pin {pin.virtual_pin} - {pin.pin_label} ({pin.pin_type})
                      </option>
                    ))}
                  </select>
                  {formData.type_block === 0 ? (
                    <p className="form-hint">
                      Chọn chân OUTPUT để điều khiển thiết bị (đèn, motor, relay...)
                    </p>
                  ) : (
                    <p className="form-hint">
                      Chọn chân INPUT để đọc dữ liệu từ cảm biến. Tạo nhiều biểu đồ để hiển thị nhiều loại dữ liệu.
                    </p>
                  )}
                </div>
              )}

              {formData.device_name && pins.length === 0 && (
                <div className="form-hint">
                  {formData.type_block === 0
                    ? 'Thiết bị này chưa có pin OUTPUT nào. Vui lòng cấu hình pin OUTPUT trong "Thiết bị của tôi".'
                    : 'Thiết bị này chưa có pin INPUT nào. Vui lòng cấu hình pin INPUT trong "Thiết bị của tôi".'}
                </div>
              )}

              {error && (
                <div className="error-alert">
                  <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                    <path
                      fillRule="evenodd"
                      d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z"
                      clipRule="evenodd"
                    />
                  </svg>
                  <span>{error}</span>
                </div>
              )}

              <div className="form-actions">
                <button type="button" className="btn-secondary" onClick={onClose} disabled={saving}>
                  Hủy
                </button>
                <button
                  type="submit"
                  className="btn-primary"
                  disabled={saving || !formData.device_name || !formData.label_block || !formData.virtual_pin}
                >
                  {saving ? 'Đang lưu...' : isNewBlock ? 'Thêm block' : 'Lưu thay đổi'}
                </button>
              </div>
            </form>
          )}
        </div>
      </div>
    </div>
  );
};

export default BlockConfigModal;
