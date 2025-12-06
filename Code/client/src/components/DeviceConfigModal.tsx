/**
 * DeviceConfigModal Component (Dumb Component)
 * Pure UI component for device pin configuration
 */

import React, { useState } from 'react';
import '../styles/DeviceConfigModal.css';
import { useDeviceConfig } from '../hooks/useDeviceConfig';
import { getDataTypeLabel, getPinTypeLabel } from '../utils/formatters';
import type { DeviceConfigModalProps, DataType, PinType } from '../types';

const DeviceConfigModal: React.FC<DeviceConfigModalProps> = ({ device, onClose }) => {
  const { configPins, loading, error, loadConfigPins, addPin, deletePin } = useDeviceConfig({
    deviceToken: device.device_token,
  });

  const [showAddPinForm, setShowAddPinForm] = useState(false);
  const [newPin, setNewPin] = useState({
    virtual_pin: 1,
    pin_label: '',
    pin_type: 'OUTPUT' as PinType,
    data_type: 'float' as DataType,
    ai_keywords: '',
  });
  const [addPinLoading, setAddPinLoading] = useState(false);
  const [addPinError, setAddPinError] = useState<string | null>(null);

  const handleAddPin = async (e: React.FormEvent) => {
    e.preventDefault();
    setAddPinError(null);

    if (!newPin.pin_label.trim()) {
      setAddPinError('Vui lòng nhập nhãn pin');
      return;
    }

    if (newPin.virtual_pin < 1 || newPin.virtual_pin > 30) {
      setAddPinError('Virtual pin phải trong khoảng 1-30');
      return;
    }

    if (newPin.pin_type === 'OUTPUT' && !newPin.ai_keywords.trim()) {
      setAddPinError('Vui lòng nhập AI keywords cho OUTPUT pin');
      return;
    }

    setAddPinLoading(true);
    try {
      await addPin({
        virtual_pin: newPin.virtual_pin,
        pin_label: newPin.pin_label.trim(),
        pin_type: newPin.pin_type,
        data_type: newPin.data_type,
        ai_keywords: newPin.pin_type === 'OUTPUT' ? newPin.ai_keywords.trim() : undefined,
      });

      setNewPin({
        virtual_pin: 1,
        pin_label: '',
        pin_type: 'OUTPUT',
        data_type: 'float',
        ai_keywords: '',
      });
      setShowAddPinForm(false);
      alert('Thêm virtual pin thành công!');
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Lỗi khi thêm virtual pin';
      setAddPinError(errorMessage);
    } finally {
      setAddPinLoading(false);
    }
  };

  const handleDeletePin = async (virtualPin: number) => {
    try {
      await deletePin(virtualPin);
    } catch {
      // Error handled by hook
    }
  };

  return (
    <div className="device-config-modal-overlay" onClick={onClose}>
      <div className="device-config-modal" onClick={(e) => e.stopPropagation()}>
        <div className="device-config-modal-header">
          <h2>Cấu hình Virtual Pin - {device.device_name}</h2>
          <button className="modal-close-btn" onClick={onClose}>
            ×
          </button>
        </div>

        <div className="device-config-modal-content">
          {loading ? (
            <div className="loading-state">Đang tải...</div>
          ) : error ? (
            <div className="error-state">
              <p>{error}</p>
              <button className="btn" onClick={loadConfigPins}>
                Thử lại
              </button>
            </div>
          ) : (
            <>
              <div className="config-pins-list">
                {configPins.length === 0 ? (
                  <div className="empty-state">
                    <p>Chưa có virtual pin nào. Hãy thêm virtual pin mới!</p>
                  </div>
                ) : (
                  <div className="pins-table">
                    <table>
                      <thead>
                        <tr>
                          <th>Virtual Pin</th>
                          <th>Nhãn Pin</th>
                          <th>Loại Pin</th>
                          <th>Kiểu dữ liệu</th>
                          <th>AI Keywords</th>
                          <th>Thao tác</th>
                        </tr>
                      </thead>
                      <tbody>
                        {configPins.map((pin, index) => (
                          <tr key={index}>
                            <td>{pin.virtual_pin}</td>
                            <td>{pin.pin_label}</td>
                            <td>{getPinTypeLabel(pin.pin_type)}</td>
                            <td>{getDataTypeLabel(pin.data_type)}</td>
                            <td>{pin.ai_keywords || '-'}</td>
                            <td>
                              <button
                                className="btn-delete-pin"
                                onClick={() => handleDeletePin(pin.virtual_pin)}
                                title="Xóa"
                              >
                                <svg width="16" height="16" viewBox="0 0 16 16" fill="none" stroke="currentColor" strokeWidth="2">
                                  <path d="M4 4h8v10a1 1 0 01-1 1H5a1 1 0 01-1-1V4zM6 2h4M2 4h12" />
                                </svg>
                              </button>
                            </td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                )}
              </div>

              {!showAddPinForm ? (
                    <div className="add-pin-button-container">
                      <button className="btn-add-pin" onClick={() => setShowAddPinForm(true)}>
                        + Thêm Virtual Pin mới
                      </button>
                    </div>
                  ) : (
                <div className="add-pin-form-container">
                  <h3>Thêm Virtual Pin mới</h3>
                  <form onSubmit={handleAddPin} className="add-pin-form">
                    <div className="form-group">
                      <label>Virtual Pin (1-30)</label>
                      <input
                        type="number"
                        min="1"
                        max="30"
                        value={newPin.virtual_pin}
                        onChange={(e) =>
                          setNewPin({ ...newPin, virtual_pin: parseInt(e.target.value) || 1 })
                        }
                        required
                        disabled={addPinLoading}
                      />
                    </div>

                    <div className="form-group">
                      <label>Nhãn Pin</label>
                      <input
                        type="text"
                        value={newPin.pin_label}
                        onChange={(e) => setNewPin({ ...newPin, pin_label: e.target.value })}
                        placeholder="Ví dụ: Bật đèn"
                        required
                        disabled={addPinLoading}
                      />
                    </div>

                    <div className="form-group">
                      <label>Loại Pin</label>
                      <select
                        value={newPin.pin_type}
                        onChange={(e) => setNewPin({ ...newPin, pin_type: e.target.value as PinType })}
                        required
                        disabled={addPinLoading}
                      >
                        <option value="OUTPUT">OUTPUT (Từ server xuống)</option>
                        <option value="INPUT">INPUT (Từ thiết bị lên)</option>
                      </select>
                    </div>

                    <div className="form-group">
                      <label>Kiểu dữ liệu</label>
                      <select
                        value={newPin.data_type}
                        onChange={(e) => setNewPin({ ...newPin, data_type: e.target.value as DataType })}
                        required
                        disabled={addPinLoading}
                      >
                        <option value="float">Float</option>
                        <option value="integer">Integer</option>
                        <option value="string">String</option>
                        <option value="boolean">Boolean</option>
                        <option value="datetime">Datetime</option>
                        <option value="json">JSON</option>
                      </select>
                    </div>

                    {newPin.pin_type === 'OUTPUT' && (
                      <div className="form-group">
                        <label>AI Keywords</label>
                        <textarea
                          value={newPin.ai_keywords}
                          onChange={(e) => setNewPin({ ...newPin, ai_keywords: e.target.value })}
                          placeholder="Ví dụ: bật đèn ở bếp, bật quạt ở trong nhà"
                          rows={3}
                          required={newPin.pin_type === 'OUTPUT'}
                          disabled={addPinLoading}
                        />
                      </div>
                    )}

                    {addPinError && <div className="error-message">{addPinError}</div>}

                    <div className="form-actions">
                      <button
                        type="button"
                        className="btn-cancel"
                        onClick={() => {
                          setShowAddPinForm(false);
                          setAddPinError(null);
                          setNewPin({
                            virtual_pin: 1,
                            pin_label: '',
                            pin_type: 'OUTPUT',
                            data_type: 'float',
                            ai_keywords: '',
                          });
                        }}
                        disabled={addPinLoading}
                      >
                        Hủy
                      </button>
                      <button type="submit" className="btn-save" disabled={addPinLoading}>
                        {addPinLoading ? 'Đang lưu...' : 'Lưu'}
                      </button>
                    </div>
                  </form>
                </div>
              )}
            </>
          )}
        </div>
      </div>
    </div>
  );
};

export default DeviceConfigModal;
