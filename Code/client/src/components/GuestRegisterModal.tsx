/**
 * GuestRegisterModal Component
 * Modal for registering guest accounts
 */

import React, { useState, useEffect } from 'react';
import '../styles/GuestRegisterModal.css';
import authService from '../services/authService';

// Types
interface GuestRegisterModalProps {
  onClose: () => void;
}

const GuestRegisterModal: React.FC<GuestRegisterModalProps> = ({ onClose }) => {
  const [formData, setFormData] = useState({
    name: '',
    email: '',
    password: '',
    confirmPassword: '',
  });
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);

  // Close modal after successful registration
  useEffect(() => {
    if (success) {
      const timer = setTimeout(() => {
        onClose();
        setFormData({
          name: '',
          email: '',
          password: '',
          confirmPassword: '',
        });
        setError(null);
        setSuccess(null);
      }, 2000);
      return () => clearTimeout(timer);
    }
  }, [success, onClose]);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError(null);
    setSuccess(null);

    // Validation
    if (!formData.name.trim() || !formData.email.trim() || !formData.password) {
      setError('Vui lòng điền đầy đủ thông tin');
      return;
    }

    if (formData.password.length < 6) {
      setError('Mật khẩu phải có ít nhất 6 ký tự');
      return;
    }

    if (formData.password !== formData.confirmPassword) {
      setError('Mật khẩu xác nhận không khớp');
      return;
    }

    setLoading(true);
    try {
      const registerData = {
        email: formData.email,
        password: formData.password,
        name: formData.name,
      };

      await authService.registerViewer(registerData);
      
      setSuccess('Tạo tài khoản guest thành công!');
      
      // Reset form
      setFormData({
        name: '',
        email: '',
        password: '',
        confirmPassword: '',
      });
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Có lỗi xảy ra khi tạo tài khoản';
      setError(errorMessage);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="guest-register-modal-overlay" onClick={onClose}>
      <div className="guest-register-modal" onClick={(e) => e.stopPropagation()}>
        <div className="guest-register-modal-header">
          <h2>Tạo tài khoản cho Guest</h2>
          <button className="modal-close-btn" onClick={onClose}>
            ×
          </button>
        </div>

        <div className="guest-register-modal-content">
          <p className="guest-register-description">
            Tạo tài khoản mới cho khách với quyền truy cập hạn chế.
          </p>

          <form onSubmit={handleSubmit} className="guest-register-form">
            <div className="form-group">
              <label className="form-label">
                <span>Họ tên</span>
                <span className="required-indicator">*</span>
              </label>
              <input
                type="text"
                className="form-input"
                value={formData.name}
                onChange={(e) => setFormData({ ...formData, name: e.target.value })}
                placeholder="Nhập họ tên"
                disabled={loading}
                required
              />
            </div>

            <div className="form-group">
              <label className="form-label">
                <span>Email</span>
                <span className="required-indicator">*</span>
              </label>
              <input
                type="email"
                className="form-input"
                value={formData.email}
                onChange={(e) => setFormData({ ...formData, email: e.target.value })}
                placeholder="email@example.com"
                disabled={loading}
                required
              />
            </div>

            <div className="form-group">
              <label className="form-label">
                <span>Mật khẩu</span>
                <span className="required-indicator">*</span>
              </label>
              <input
                type="password"
                className="form-input"
                value={formData.password}
                onChange={(e) => setFormData({ ...formData, password: e.target.value })}
                placeholder="Tối thiểu 6 ký tự"
                disabled={loading}
                required
                minLength={6}
              />
            </div>

            <div className="form-group">
              <label className="form-label">
                <span>Xác nhận mật khẩu</span>
                <span className="required-indicator">*</span>
              </label>
              <input
                type="password"
                className="form-input"
                value={formData.confirmPassword}
                onChange={(e) => setFormData({ ...formData, confirmPassword: e.target.value })}
                placeholder="Nhập lại mật khẩu"
                disabled={loading}
                required
              />
            </div>

            {error && (
              <div className="form-error">
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

            {success && (
              <div className="form-success">
                <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                  <path
                    fillRule="evenodd"
                    d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z"
                    clipRule="evenodd"
                  />
                </svg>
                <span>{success}</span>
              </div>
            )}

            <div className="form-actions">
              <button
                type="button"
                className="btn-secondary"
                onClick={onClose}
                disabled={loading}
              >
                Hủy
              </button>
              <button type="submit" className="btn-primary" disabled={loading}>
                {loading ? 'Đang tạo...' : 'Tạo tài khoản'}
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
};

export default GuestRegisterModal;
