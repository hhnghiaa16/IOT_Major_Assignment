/**
 * AccountDropdown Component
 * Dropdown menu for account actions
 */

import React, { useState, useRef, useEffect } from 'react';
import '../styles/AccountDropdown.css';
import ProfileModal from './ProfileModal';
import GuestRegisterModal from './GuestRegisterModal';
// Types
interface User {
  id: number;
  email: string;
  name: string;
  type?: number;
}

interface AccountDropdownProps {
  onLogout: () => void;
}

const AccountDropdown: React.FC<AccountDropdownProps> = ({ onLogout }) => {
  const [isOpen, setIsOpen] = useState(false);
  const [showProfileModal, setShowProfileModal] = useState(false);
  const [showGuestModal, setShowGuestModal] = useState(false);
  const [user, setUser] = useState<User | null>(null);
  const dropdownRef = useRef<HTMLDivElement>(null);

  // Load user from localStorage
  useEffect(() => {
    const userStr = localStorage.getItem('user');
    if (userStr) {
      try {
        setUser(JSON.parse(userStr));
      } catch {
        setUser(null);
      }
    }
  }, []);

  // Handle click outside to close dropdown
  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (dropdownRef.current && !dropdownRef.current.contains(event.target as Node)) {
        setIsOpen(false);
      }
    };

    if (isOpen) {
      document.addEventListener('mousedown', handleClickOutside);
    }

    return () => {
      document.removeEventListener('mousedown', handleClickOutside);
    };
  }, [isOpen]);

  const getInitials = (name: string | undefined | null): string => {
    if (!name || !name.trim()) {
      return 'U';
    }
    return name
      .split(' ')
      .map((n) => n[0])
      .join('')
      .toUpperCase()
      .slice(0, 2);
  };

  const getUserDisplayName = (name: string | undefined | null): string => {
    return name || 'User';
  };

  return (
    <>
      <div className="account-dropdown" ref={dropdownRef}>
        <button className="account-trigger" onClick={() => setIsOpen(!isOpen)}>
          <div className="account-avatar">
            {getInitials(user?.name)}
          </div>
          <svg
            className={`account-chevron ${isOpen ? 'open' : ''}`}
            width="16"
            height="16"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            strokeWidth="2"
          >
            <path d="M6 9l6 6 6-6" />
          </svg>
        </button>

        {isOpen && (
          <div className="account-menu">
            <div className="account-menu-header">
              <div className="account-menu-avatar">
                {getInitials(user?.name)}
              </div>
              <div className="account-menu-info">
                <div className="account-menu-name">{getUserDisplayName(user?.name)}</div>
                <div className="account-menu-email">{user?.email || ''}</div>
              </div>
            </div>

            <div className="account-menu-divider"></div>

            <button
              className="account-menu-item"
              onClick={() => {
                setShowProfileModal(true);
                setIsOpen(false);
              }}
            >
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2" />
                <circle cx="12" cy="7" r="4" />
              </svg>
              <span>Xem thông tin cá nhân</span>
            </button>

            <button
              className="account-menu-item"
              onClick={() => {
                setShowGuestModal(true);
                setIsOpen(false);
              }}
            >
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M16 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2" />
                <circle cx="8.5" cy="7" r="4" />
                <path d="M20 8v6M23 11h-6" />
              </svg>
              <span>Tạo tài khoản cho guest</span>
            </button>

            <div className="account-menu-divider"></div>

            <button className="account-menu-item account-menu-item-logout" onClick={onLogout}>
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4M16 17l5-5-5-5M21 12H9" />
              </svg>
              <span>Đăng xuất</span>
            </button>
          </div>
        )}
      </div>

      {showProfileModal && (
        <ProfileModal onClose={() => setShowProfileModal(false)} />
      )}

      {showGuestModal && (
        <GuestRegisterModal onClose={() => setShowGuestModal(false)} />
      )}
    </>
  );
};

export default AccountDropdown;
