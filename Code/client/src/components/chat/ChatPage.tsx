import React, { useState } from 'react';
import '../../styles/ChatPage.css';
import AIConfigPanel from './AIConfigPanel';
import ChatInterface from './ChatInterface';

const ChatPage: React.FC = () => {
    const [aiName, setAiName] = useState('AI Support');
    const [aiStyle, setAiStyle] = useState('Thân thiện, nhiệt tình');

    const handleConfigChange = (name: string, style: string) => {
        setAiName(name);
        setAiStyle(style);
    };

    return (
        <div className="chat-page">
            {/* Left Sidebar: AI Configuration */}
            <AIConfigPanel onConfigChange={handleConfigChange} />

            {/* Right Content: Chat Interface */}
            <ChatInterface aiName={aiName} aiStyle={aiStyle} />
        </div>
    );
};

export default ChatPage;