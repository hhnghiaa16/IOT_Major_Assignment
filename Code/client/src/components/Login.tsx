/**
 * Login Component (Dumb Component)
 * Pure UI component for authentication
 */

import React from 'react';
import '../styles/Login.css';
import { useLogin } from '../hooks/useLogin';
import type { LoginProps } from '../types';

const Login: React.FC<LoginProps> = ({ onLogin }) => {
  const {
    mode,
    setMode,
    email,
    setEmail,
    name,
    setName,
    password,
    setPassword,
    loading,
    error,
    success,
    resetMessages,
    handleSubmit,
  } = useLogin();

  return (
    <div className="login-container">
      <div className="login-card">
        <div className="login-header">
          <div className="login-header-icon">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" width="24" height="24">
              <path
                strokeLinecap="round"
                strokeLinejoin="round"
                strokeWidth={2}
                d="M12 11c0-1.657 1.343-3 3-3s3 1.343 3 3v2a3 3 0 01-3 3h-6a3 3 0 01-3-3v-2c0-1.657 1.343-3 3-3s3 1.343 3 3z"
              />
            </svg>
          </div>
          <div>
            <div className="login-header-title">Bảng điều khiển IoT</div>
            <div className="login-header-sub">Quản lý thiết bị và truy cập hệ thống</div>
          </div>
        </div>

        <div style={{ padding: '0 0.25rem 1rem 0.25rem' }}>
          <h2 style={{ fontSize: '1.75rem', margin: '0 0 4px 0' }}>
            {mode === 'login' ? 'Đăng nhập' : 'Tạo tài khoản'}
          </h2>
          <p className="lead" style={{ marginBottom: '0.75rem' }}>
            {mode === 'login' ? 'Nhập thông tin tài khoản của bạn' : 'Tạo tài khoản mới để bắt đầu'}
          </p>
        </div>

        {error && <div className="error">{error}</div>}
        {success && <div className="success">{success}</div>}

        <form
          onSubmit={(e) => handleSubmit(e, onLogin)}
          className="login-form"
          style={{ marginTop: 6 }}
        >
          {mode === 'register' && (
            <div className="form-group">
              <label>Họ tên</label>
              <input
                type="text"
                value={name}
                onChange={(e) => setName(e.target.value)}
                placeholder="Tên hiển thị"
                disabled={loading}
              />
            </div>
          )}

          <div className="form-group">
            <label>Email</label>
            <input
              type="email"
              value={email}
              onChange={(e) => setEmail(e.target.value)}
              placeholder="you@example.com"
              disabled={loading}
            />
          </div>

          <div className="form-group">
            <label>Mật khẩu</label>
            <input
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              placeholder="Mật khẩu"
              disabled={loading}
            />
          </div>

          <div className="form-actions">
            <button type="submit" disabled={loading} className="btn primary">
              {loading ? 'Đang xử lý...' : mode === 'login' ? 'Đăng nhập' : 'Tạo tài khoản'}
            </button>

            <button
              type="button"
              onClick={() => {
                resetMessages();
                setMode(mode === 'login' ? 'register' : 'login');
              }}
              disabled={loading}
              className="btn link"
            >
              {mode === 'login' ? 'Chưa có tài khoản? Tạo mới' : 'Đã có tài khoản? Đăng nhập'}
            </button>
          </div>

          <div className="login-note"></div>
        </form>

        <div className="login-footer">
          Bản quyền © {new Date().getFullYear()} — Hệ thống IoT
        </div>
      </div>
    </div>
  );
};

export default Login;
